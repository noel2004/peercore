#include "sctp.h"
#include "usrsctplib/usrsctp.h"
#include "glog/logging.h"
#include <boost/noncopyable.hpp>
#include <map>

template<class Layer>
class SCTPPrepare : public SCTP_base
{
    Layer& lowLevelTransLayer_;

    void onLowLayerIO(bool read, const boost::system::error_code&, std::size_t);

    //interface
    void onDispatchToLower(const boost::asio::mutable_buffer&) override;
public:
    SCTPPrepare(Layer& l) : lowLevelTransLayer_(l){}
};



namespace rtcdc {

class SCTPModuleImpl : public SCTPModule
{
    static int sctp_conn_output(void *addr, void *buf, size_t length, uint8_t tos, uint8_t set_df)
    {
        auto pbase = (SCTP_base*)(addr);

    }

    struct portRegData
    {
        boost::shared_ptr<SCTP_base> base_;
    };

    std::map<int, portRegData>  portRegIndex_;
    std::map<SCTP_base*, int>   regIndex_;

    bool regSCTPLocalPort(int port, SCTP_base*) override
    {
        auto f = portRegIndex_.find(port);
        if (f != portRegIndex_.end())return false;
    }

public:
    SCTPModuleImpl()
    {
    }

    ~SCTPModuleImpl()
    {
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