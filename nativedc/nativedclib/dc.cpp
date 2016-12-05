#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "dc.h"
#include "dctransport.h"
#include "usrsctplib/usrsctp.h"
#include "glog/logging.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <map>

namespace rtcdc
{

namespace {

    class DataChannelCoreImpl : DatachannelCoreCall, 
        public boost::enable_shared_from_this<DataChannelCoreImpl>
    {
        boost::shared_mutex dc_index_lock_;
        typedef boost::shared_lock<boost::shared_mutex> read_lock;
        typedef boost::upgrade_lock<boost::shared_mutex> write_lock;
        std::map<unsigned short, boost::shared_ptr<DataChannel> > dc_index_;

        void    onAssocError(const std::string& reason, bool closed) final 
        {
            LOG(INFO) << "Get association error: " << reason << 
                (closed ? " (fatal)" : " (general)") << std::endl;
        }

        void    onAssocOpened(unsigned short streams[2]) final 
        {
        }

        void    onAssocClosed() final 
        {
        }

        void    onCanSendMore() final 
        {
        }

        void    onDCMessage(unsigned short sid, unsigned int ppid, 
            const boost::asio::mutable_buffer& buffer, void (*freebuffer)(void*)) final 
        {
            boost::shared_ptr<DataChannel> p;
            try{
                read_lock guard(dc_index_lock_);
                auto f = dc_index_.find(sid);
                if (f != dc_index_.end())p = f->second;
            }
            catch (boost::lock_error& e)
            {
                LOG(ERROR) << "Get read lock fail! lost message for [" << sid <<"]: "
                    <<e.what()<< std::endl;
            }
            
            if (!p)
            {
                //send some error ack?
            }
            else {
                //deliver to dc ....

            }
        }

        boost::asio::io_service&    io_srv_;

    public:
        DataChannelCoreImpl(boost::asio::io_service& ios) : io_srv_(ios)
        {

        }
    };

}




}//namespace rtcdc