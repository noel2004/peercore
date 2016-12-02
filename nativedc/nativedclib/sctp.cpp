#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "sctp.h"
#include "dctransport.h"
#include "usrsctplib/usrsctp.h"
#include "glog/logging.h"
#include <boost/asio/buffer.hpp>
#include <boost/thread/thread.hpp>
#include <boost/atomic.hpp>
#include <cstdarg>
#include <exception>

namespace rtcdc { namespace sctp{

/*------------------------------- SOCKETCORE --------------------------------*/

SocketCore::~SocketCore(){}

class SCTPDemultiplexFail : std::runtime_error
{
public:
    SCTPDemultiplexFail() : std::runtime_error("Not existed SCTP port"){}
};

namespace _concept{

    struct callbackhandler
    {
        boost::shared_ptr<DatachannelCoreCall> demultiplex(unsigned short port) throw(SCTPDemultiplexFail);
        int not_handled(struct socket *, unsigned short port, void *, size_t, 
            const struct sctp_rcvinfo&, int);
    };
}

template<class RecvCallbackHandler>
class SocketCoreImpl : public SocketCore
{
protected:
    inline void bufferFree(void* data) { free(data); }

    static int usrsctp_receive_cb(struct socket *sock, union sctp_sockstore addr, void *data,
        size_t datalen, struct sctp_rcvinfo rcv, int flags, void *ulp_info)
    {
        //no data (data == nullptr) may indicate a error in socket 

        //lazy binding ...
        bool handled = false;
        auto demultiplex = [&handled, &addr, ulp_info]()-> boost::shared_ptr<DatachannelCoreCall>
        {
            handled = true;
            return reinterpret_cast<RecvCallbackHandler*>(ulp_info)->demultiplex(addr.sconn.sconn_port);
        };

        try {
            if (data == nullptr) {
                demultiplex()->onAssocError("Recv error: no data");
            }
            else if ((flags & MSG_NOTIFICATION) != 0)
            {
                //handle notification ...
                auto pnotify = (union sctp_notification *) (data);
                switch (pnotify->sn_header.sn_type)
                {
                case SCTP_ASSOC_CHANGE:
                {
                    auto& sac = pnotify->sn_assoc_change;
                    switch (sac.sac_state)
                    {
                    case SCTP_COMM_UP:
                    {                    
                        unsigned short strmcnt[2] = {sac.sac_inbound_streams, 
                            sac.sac_outbound_streams};
                        demultiplex()->onAssocOpened(strmcnt);
                        break;
                    }
                    case SCTP_SHUTDOWN_COMP:
                        demultiplex()->onAssocClosed();
                        break;
                    case SCTP_COMM_LOST:
                        demultiplex()->onAssocError("Lost", true);
                        break;
                    case SCTP_CANT_STR_ASSOC:
                        demultiplex()->onAssocError("Can't setup");
                        break;
                    }
                }
                    break;
                case SCTP_SEND_FAILED_EVENT:
                    break;
                case SCTP_SENDER_DRY_EVENT:
                    demultiplex()->onCanSendMore();
                    break;
                }
            }
            else {

            }

        }
        catch (SCTPDemultiplexFail& e)
        {
            handled = true;
        }

        if (!handled)
        {
            reinterpret_cast<RecvCallbackHandler*>(ulp_info)->not_handled(sock, 
            addr.sconn.sconn_port, data, datalen, rcv, flags);
        }

        if (data != nullptr)free(data);//IMPORTANT! the data in callback is malloced! 
                                       //(and possible performance penalty ...)
        return 1;
    }

    struct socket*  usrsctp_sock_;
    const bool      is_one_to_one_;

public:
    SocketCoreImpl(bool one_to_one) : 
        is_one_to_one_(one_to_one), usrsctp_sock_(nullptr){}

    bool    init(sctp::AssociationBase* base, unsigned short sctpport)
    {
        usrsctp_sock_ = usrsctp_socket(AF_CONN, is_one_to_one_ ? SOCK_STREAM : SOCK_SEQPACKET, 
            IPPROTO_SCTP, usrsctp_receive_cb, NULL, 0, static_cast<RecvCallbackHandler*>(this));
        if (usrsctp_sock_ == nullptr)
        {
            LOG(ERROR) << "create sctp socket failed: " << errno << std::endl;
            return false;
        }

        do
        {
            LOG(INFO) << "bind sctp socket to AF_CONN addr: "<<base->layerName()<<"@"<<sctpport<<" ..."<< std::endl;
            struct sockaddr_conn sconn = { 0 };
            sconn.sconn_family = AF_CONN;
            sconn.sconn_addr = base;
            sconn.sconn_port = htons(sctpport);
            if (usrsctp_bind(usrsctp_sock_, (struct sockaddr *)&sconn, sizeof(struct sockaddr_conn)) < 0)
            {
                LOG(ERROR) << "bind fail: " << errno << std::endl;
                break;
            }

            struct sctp_event sctpevent = { 0 };
            sctpevent.se_assoc_id = SCTP_ALL_ASSOC;
            sctpevent.se_on = 1;
            uint16_t event_types[] = {
                SCTP_ASSOC_CHANGE, 
                SCTP_SEND_FAILED_EVENT, 
                SCTP_SENDER_DRY_EVENT
            };
            for (int i = 0; i < sizeof(event_types) / sizeof(uint16_t); i++)
            {
		        sctpevent.se_type = event_types[i];
		        if (usrsctp_setsockopt(usrsctp_sock_, IPPROTO_SCTP, SCTP_EVENT, &sctpevent, 
                    sizeof(sctpevent)) < 0) {
			        LOG(ERROR) << "subscribe event "<<sctpevent.se_type<<" fail:"<< errno << std::endl;
                    break;
		        }
            }

            //backlog = 1 can be applied to any mode
            if (usrsctp_listen(usrsctp_sock_, 1) < 0)
            {
                LOG(ERROR) << "listen fail: " << errno << std::endl;
                break;
            }
               
            return true;
        } while (false);

        usrsctp_close(usrsctp_sock_);
        usrsctp_sock_ = nullptr;
        return false;
    }

    ~SocketCoreImpl()
    {
        if(usrsctp_sock_ != nullptr){
            //for safety ...
            usrsctp_set_ulpinfo(usrsctp_sock_, nullptr);
            usrsctp_close(usrsctp_sock_);
        }
    }

    void*   nativeHandle() override{return usrsctp_sock_;}
    bool    oneToManyMode() const override{return !is_one_to_one_;}
};


class CollectionSocketCore : public SocketCoreImpl<CollectionSocketCore>
{
    boost::mutex    peeloff_lock_;
    boost::mutex    demultiplex_lock_;
public:
    CollectionSocketCore() : SocketCoreImpl<CollectionSocketCore>(false){}

    int f(struct socket * sock, unsigned short port, void *, size_t,
        const struct sctp_rcvinfo&, int)
    {
        //TODO:
        //one - to -many socket must handle the case that a association is peeloff:
        //in this case the sock (may) not identify to the aggregated one (usrsctp_sock_)
        if (sock != usrsctp_sock_)
        {
            boost::mutex::scoped_lock guard(peeloff_lock_);
            return;
        }

        //must demultiplex the content to given dc-core
        //TODO
    }

    bool    addEntry(unsigned short port, boost::shared_ptr<DatachannelCoreCall>) final
    {
        //TODO
        return false;
    }

    boost::shared_ptr<SocketCore>   peeloff() final
    {
        return nullptr;
    }
};

class SingleSocketCore : public SocketCoreImpl<SingleSocketCore>
{
    boost::mutex                        accept_lock_;
    boost::atomic<DatachannelCoreCall*> callback_;
public:
    SingleSocketCore() : SocketCoreImpl<CollectionSocketCore>(true){}

    int f(struct socket * sock, unsigned short /*omit the port*/, 
        void *data, size_t datalen, const struct sctp_rcvinfo& rcvinfo, int flags)
    {
    }

    bool    addEntry(unsigned short port, boost::shared_ptr<DatachannelCoreCall>) final
    {
        //TODO
        return false;
    }

    boost::shared_ptr<SocketCore>   peeloff() final{
        LOG(ERROR) << "call peeloff on one-to-one socket" << std::endl;
        return nullptr;
    }
};

/*------------------------------- ASSOCIATION --------------------------------*/

AssociationBase::AssociationBase()
{
    usrsctp_register_address(this);
}

AssociationBase::~AssociationBase()
{
    usrsctp_deregister_address(this);
}

void     AssociationBase::onRecvFromToLower(const unsigned char* p, size_t sz, short ecn_bits)
{
    usrsctp_conninput(this, p, sz, (uint8_t) ecn_bits);
}

/*------------------------------- MODULE --------------------------------*/

class ModuleImpl : public Module
{
    static int sctp_conn_output(void *addr, void *buf, size_t length, uint8_t tos, uint8_t set_df)
    {
        auto pbase = (AssociationBase*)(addr);
        return pbase->onDispatchToLower(boost::asio::buffer(buf, length), tos, set_df);
    }

    static void sctp_debug_out(const char *format, ...)
    {
        va_list ap;
        const static int bufMaxSize = 512;
        char    tempBuffer[bufMaxSize];

	    va_start(ap, format);
        VLOG(5) << (vsnprintf(tempBuffer, bufMaxSize, format, ap), tempBuffer);	    
	    va_end(ap);
    }

public:
    ModuleImpl()
    {
        usrsctp_init(0, sctp_conn_output, sctp_debug_out);
    }

    ~ModuleImpl()
    {
        LOG(INFO) << "Releasing sctp stack ..." << std::endl;
        while(usrsctp_finish() != 0)
            boost::this_thread::sleep_for(boost::chrono::milliseconds(5));
        LOG(INFO) << "Release sctp stack done!" << std::endl;
    }

    static boost::atomic<Module*> instance_;
    static boost::mutex instantiation_mutex_;
};

boost::atomic<Module*> ModuleImpl::instance_(nullptr);
boost::mutex ModuleImpl::instantiation_mutex_;

Module* Module::instance()
{
    auto tmp = ModuleImpl::instance_.load(boost::memory_order_consume);
    if (tmp == nullptr)
    {
        boost::mutex::scoped_lock guard(ModuleImpl::instantiation_mutex_);
        tmp = ModuleImpl::instance_.load(boost::memory_order_consume);
        if (tmp == nullptr)
        {
            tmp = new ModuleImpl;
            ModuleImpl::instance_.store(tmp, boost::memory_order_release);
        }
    }

    return tmp;
}

void Module::Init(bool release)
{
    if(!release)ModuleImpl::instance_.store(new ModuleImpl);
    else delete ModuleImpl::instance_.exchange(nullptr);
}


Module::~Module(){}

}}//namespace rtcdc::sctp