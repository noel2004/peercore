#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "sctp.h"
#include "usrsctplib/usrsctp.h"
#include "glog/logging.h"
#include <boost/asio/buffer.hpp>
#include <boost/thread/thread.hpp>
#include <boost/atomic.hpp>
#include <cstdarg>

namespace rtcdc { namespace sctp{

/*------------------------------- SOCKETCORE --------------------------------*/

SocketCore::~SocketCore(){}

namespace _concept{

    struct callbackhandler
    {
        int f(struct socket *, unsigned short port, void *, size_t, 
            const struct sctp_rcvinfo&, int);
    };
}

template<class RecvCallbackHandler>
class SocketCoreImpl : public SocketCore
{
protected:
    static int usrsctp_receive_cb(struct socket *sock, union sctp_sockstore addr, void *data,
        size_t datalen, struct sctp_rcvinfo rcv, int flags, void *ulp_info)
    {
        reinterpret_cast<RecvCallbackHandler*>(ulp_info)->f(sock, 
            addr.sconn.sconn_port, data, datalen, rcv, flags);
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
            sctpevent.se_assoc_id = 0;

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

    bool    addEntry(unsigned short port, DatachannelCoreCall*) final
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
    boost::mutex    accept_lock_;
public:
    SingleSocketCore() : SocketCoreImpl<CollectionSocketCore>(true){}

    int f(struct socket * sock, unsigned short , void *, size_t,
        const struct sctp_rcvinfo&, int)
    {
    }

    bool    addEntry(unsigned short port, DatachannelCoreCall*) final
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