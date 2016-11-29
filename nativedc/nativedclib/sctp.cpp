#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "sctp.h"
#include "usrsctplib/usrsctp.h"
#include "glog/logging.h"
#include <boost/thread/thread.hpp>
#include <cstdarg>

namespace rtcdc { namespace sctp{

bool    SocketBase::errorIsWouldBlock(int e) { return e == EWOULDBLOCK; }

namespace {

    int usrsctp_receive_cb(struct socket *sock, union sctp_sockstore addr, void *data,
        size_t datalen, struct sctp_rcvinfo rcv, int flags, void *ulp_info)
    {
        
    }

}

SocketBase::SocketBase()
{
    usrsctp_sock_ = usrsctp_socket(AF_CONN, SOCK_STREAM, IPPROTO_SCTP, usrsctp_receive_cb, NULL, 0, this);
}

SocketBase::~SocketBase()
{

}

int     SocketBase::sendv(unsigned short sid, const boost::asio::mutable_buffer&, unsigned int ppid, int rtx = 0, bool ord = false)
{

}

int     SocketBase::sendv(const boost::asio::mutable_buffer&)
{

}

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
};

boost::atomic<Module*> Module::instance_(nullptr);
boost::mutex Module::instantiation_mutex_;

Module* Module::instance()
{
    auto tmp = instance_.load(boost::memory_order_consume);
    if (tmp == nullptr)
    {
        boost::mutex::scoped_lock guard(instantiation_mutex_);
        tmp = instance_.load(boost::memory_order_consume);
        if (tmp == nullptr)
        {
            tmp = new ModuleImpl;
            instance_.store(tmp, boost::memory_order_release);
        }
    }

    return tmp;
}

void Module::Init(bool release)
{
    if(!release)instance_.store(new ModuleImpl);
    else delete instance_.exchange(nullptr);
}


Module::~Module(){}

}}//namespace rtcdc::sctp