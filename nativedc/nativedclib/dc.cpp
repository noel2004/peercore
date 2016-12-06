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

namespace {
    //DataChannel constants: 
    const unsigned int DATA_CHANNEL_PPID_CONTROL = 50;
    const unsigned int DATA_CHANNEL_PPID_DOMSTRING = 51;
    const unsigned int DATA_CHANNEL_PPID_BINARY = 52;

    const unsigned short  DATA_CHANNEL_CLOSED = 0;
    const unsigned short  DATA_CHANNEL_CONNECTING = 1;
    const unsigned short  DATA_CHANNEL_OPEN = 2;
    const unsigned short  DATA_CHANNEL_CLOSING = 3;

    const unsigned int DATA_CHANNEL_FLAGS_SEND_REQ = 0x00000001u;
    const unsigned int DATA_CHANNEL_FLAGS_SEND_RSP = 0x00000002u;
    const unsigned int DATA_CHANNEL_FLAGS_SEND_ACK = 0x00000004u;

    const uint8_t DATA_CHANNEL_OPEN_REQUEST = 0;
    const uint8_t DATA_CHANNEL_OPEN_RESPONSE = 1;
    const uint8_t DATA_CHANNEL_ACK = 2;

    const uint8_t DATA_CHANNEL_RELIABLE = 0;
    const uint8_t DATA_CHANNEL_RELIABLE_STREAM = 1;
    const uint8_t DATA_CHANNEL_UNRELIABLE = 2;
    const uint8_t DATA_CHANNEL_PARTIAL_RELIABLE_REXMIT = 3;
    const uint8_t DATA_CHANNEL_PARTIAL_RELIABLE_TIMED = 4;

    const unsigned short DATA_CHANNEL_FLAG_OUT_OF_ORDER_ALLOWED = 0x0001u;

    #ifndef _WIN32
    #define DC_PACKED __attribute__((packed))
    #else
    #pragma pack (push, 1)
    #define DC_PACKED
    #endif

    struct rtcweb_datachannel_open_request {
	    uint8_t msg_type; /* DATA_CHANNEL_OPEN_REQUEST */
	    uint8_t channel_type;
	    uint16_t flags;
	    uint16_t reliability_params;
	    int16_t priority;
	    char label[];
    } DC_PACKED;

    struct rtcweb_datachannel_open_response {
	    uint8_t  msg_type; /* DATA_CHANNEL_OPEN_RESPONSE */
	    uint8_t  error;
	    uint16_t flags;
	    uint16_t reverse_stream;
    } DC_PACKED;

    struct rtcweb_datachannel_ack {
	    uint8_t  msg_type; /* DATA_CHANNEL_ACK */
    } DC_PACKED;

    #ifdef _WIN32
    #pragma pack()
    #endif

    #undef DC_PACKED

}

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
                switch (ppid) {
                case DATA_CHANNEL_PPID_CONTROL:
                    if (boost::asio::buffer_size(buffer) < sizeof(rtcweb_datachannel_ack))
                    {
                        VLOG_EVERY_N(3, 128) << "Receive invalid message for DC Control: size is "
                            << boost::asio::buffer_size(buffer) <<" bytes" << std::endl;
                        return;
                    }

                    switch(boost::asio::buffer_cast<rtcweb_datachannel_ack*>(buffer)->msg_type){
                    case DATA_CHANNEL_OPEN_REQUEST:
                    case DATA_CHANNEL_OPEN_RESPONSE:
                    case DATA_CHANNEL_ACK:
                        break;
                    default:
                        break;
                    }

                    break;
                }
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