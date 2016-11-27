#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "sctp.h"
#include "usrsctplib/usrsctp.h"
#include "glog/logging.h"
#include <boost/thread/thread.hpp>
#include <cstdarg>

namespace rtcdc {

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

class SCTPModuleImpl : public SCTPModule
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
    SCTPModuleImpl()
    {
        usrsctp_init(0, sctp_conn_output, sctp_debug_out);
    }

    ~SCTPModuleImpl()
    {
        LOG(INFO) << "Releasing sctp stack ..." << std::endl;
        while(usrsctp_finish() != 0)
            boost::this_thread::sleep_for(boost::chrono::milliseconds(5));
        LOG(INFO) << "Release sctp stack done!" << std::endl;
    }
};

boost::atomic<SCTPModule*> SCTPModule::instance_(nullptr);
boost::mutex SCTPModule::instantiation_mutex_;

SCTPModule* SCTPModule::instance()
{
    auto tmp = instance_.load(boost::memory_order_consume);
    if (tmp == nullptr)
    {
        boost::mutex::scoped_lock guard(instantiation_mutex_);
        tmp = instance_.load(boost::memory_order_consume);
        if (tmp == nullptr)
        {
            tmp = new SCTPModuleImpl;
            instance_.store(tmp, boost::memory_order_release);
        }
    }

    return tmp;
}

void SCTPModule::SCTPInit(bool release)
{
    if(!release)instance_.store(new SCTPModuleImpl);
    else delete instance_.exchange(nullptr);
}


SCTPModule::~SCTPModule(){}

}//namespace rtcdc