#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "dc.h"
#include "dctransport.h"
#include "sctp.h"
#include "usrsctplib/usrsctp.h"
#include "glog/logging.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <unordered_map>
#include <exception>

namespace {
    //DataChannel constants: 
    const unsigned int DATA_CHANNEL_PPID_CONTROL = 50;
    const unsigned int DATA_CHANNEL_PPID_DOMSTRING = 51;
    const unsigned int DATA_CHANNEL_PPID_PARTIAL_BINARY = 52;//WebRTC "Binary Partial", deprecated
    const unsigned int DATA_CHANNEL_PPID_PARTIAL_DOMSTRING = 54;//WebRTC "String partial", deprecated
    const unsigned int DATA_CHANNEL_PPID_BINARY = 53;
    const unsigned int DATA_CHANNEL_PPID_EMPTY_STRING = 56;
    const unsigned int DATA_CHANNEL_PPID_EMPTY_BINARY = 57;

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
    const uint8_t DATA_CHANNEL_OPEN_NEW = 3;

    //*** this defination follows draft-jesup-data-protocol-03 and later and
    //*** is not compatible with draft-01 or 02
    const uint8_t DATA_CHANNEL_RELIABLE = 0;
    //const uint8_t DATA_CHANNEL_RELIABLE_STREAM = 1;
    //const uint8_t DATA_CHANNEL_UNRELIABLE = 2;
    const uint8_t DATA_CHANNEL_PARTIAL_RELIABLE_REXMIT = 1;
    const uint8_t DATA_CHANNEL_PARTIAL_RELIABLE_TIMED = 2;

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
//	    char label[];
    } DC_PACKED;

    struct rtcweb_datachannel_open_msg {
	    uint8_t msg_type; /* DATA_CHANNEL_OPEN_REQUEST */
	    uint8_t channel_type;
	    uint32_t reliability_params;
	    int16_t priority;
        uint16_t laben_len;
        uint16_t protocol_len;
//	    char label_protocol[];
    } DC_PACKED;

    struct rtcweb_datachannel_open_response {
	    uint8_t  msg_type; /* DATA_CHANNEL_OPEN_RESPONSE */
	    uint8_t  error;
	    uint16_t flags;
	    uint16_t reverse_stream;
    } DC_PACKED;

    //not used in fact, we simply read the first byte in buffer
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

    class DC_Invalid_Request : public std::invalid_argument
    {
        static std::string dump(rtcweb_datachannel_open_request*);
        static std::string dump(rtcweb_datachannel_open_msg*);
    public:
        template<class T>
        explicit DC_Invalid_Request(T* p) : std::invalid_argument(dump(p)){}
        DC_Invalid_Request(size_t /*len*/, size_t /*expect*/) 
            : std::invalid_argument("Unexpected open request size")
        {}
    };

    class DataChannelImpl : public DataChannel, boost::noncopyable
    {
        struct   socket *sctp_socket_;
        uint16_t sctp_pr_policy_;
        bool     updateChnType(uint8_t chntype)
        {
            switch (chntype)
            {
            case DATA_CHANNEL_RELIABLE:
                sctp_pr_policy_ = SCTP_PR_SCTP_NONE;
                break;
            case DATA_CHANNEL_PARTIAL_RELIABLE_REXMIT:
                sctp_pr_policy_ = SCTP_PR_SCTP_RTX;
                break;
            case DATA_CHANNEL_PARTIAL_RELIABLE_TIMED:
                sctp_pr_policy_ = SCTP_PR_SCTP_TTL;
                break;
            default:
                LOG(ERROR) << "unspecified channel type " << chntype << std::endl;
                return false;
            }
            return true;
        }

    public:

        const bool jesup_compatible_;

        DataChannelImpl() : jesup_compatible_(false), sctp_socket_(nullptr)
            /*not care the flag for initiator*/
        {

        }

        DataChannelImpl(struct rtcweb_datachannel_open_request* p, size_t len) throw(DC_Invalid_Request)
        : jesup_compatible_(true), sctp_socket_(nullptr){
            if (len < sizeof(struct rtcweb_datachannel_open_request))
                throw DC_Invalid_Request(len, sizeof(struct rtcweb_datachannel_open_request));

            if (!updateChnType(p->channel_type))throw DC_Invalid_Request(p);
        }

        DataChannelImpl(struct rtcweb_datachannel_open_msg* p, size_t len) throw(DC_Invalid_Request)
        : jesup_compatible_(false), sctp_socket_(nullptr)
        {

        }

        void    setSocket(struct socket* s) { sctp_socket_ = s; }
    };

    class DataChannelCoreImpl : DatachannelCoreCall, 
        public boost::enable_shared_from_this<DataChannelCoreImpl>
    {
        std::unordered_map<unsigned short, boost::shared_ptr<DataChannelImpl> > dc_index_;

        boost::shared_ptr<sctp::SocketCore>          bind_socket_;

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
            auto ref = shared_from_this();
            io_srv_.post([this, sid, ppid, buffer, freebuffer, ref]()
            {
                boost::shared_ptr<DataChannelImpl> p;
                auto f = dc_index_.find(sid);
                if (f != dc_index_.end())p = f->second;
                auto buffersz = boost::asio::buffer_size(buffer);

                switch (ppid) {
                case DATA_CHANNEL_PPID_CONTROL:
                    if (buffersz < sizeof(rtcweb_datachannel_ack))
                    {
                        VLOG_EVERY_N(3, 128) << "Receive invalid message for DC Control: size is "
                            << boost::asio::buffer_size(buffer) <<" bytes" << std::endl;
                        return;
                    }

                    switch(*boost::asio::buffer_cast<uint8_t*>(buffer)){
                    //try to support both old draft-rtcweb-data-protocol (jesup ver.) and the renewed one (rtcweb)
                    case DATA_CHANNEL_OPEN_NEW:
                    case DATA_CHANNEL_OPEN_REQUEST:
                        if (!!p) {
                            LOG(ERROR) << "channel " << sid << " has existed " << std::endl;
                            //cancel the exist channel
                             dcctrl_close(p);
                        }
                        else {
                            try {
                                p = boost::shared_ptr<DataChannelImpl>(
                                    *boost::asio::buffer_cast<uint8_t*>(buffer) == DATA_CHANNEL_OPEN_REQUEST ?
                                    new DataChannelImpl(boost::asio::buffer_cast<struct rtcweb_datachannel_open_request*>(buffer), buffersz) :
                                    new DataChannelImpl(boost::asio::buffer_cast<struct rtcweb_datachannel_open_msg*>(buffer), buffersz) );
                                 dcctrl_open(p);
                            }
                            catch (DC_Invalid_Request& e)
                            {
                                LOG(ERROR) << "handle datachannel open request fail: "
                                    << e.what() << std::endl;
                                break;
                            }
                        }
                        break;
                    case DATA_CHANNEL_OPEN_RESPONSE:
                    {
                        auto pmsg = boost::asio::buffer_cast<struct rtcweb_datachannel_open_response*>(buffer);
                        //error and flag in response is TBD. and not used
                        if (pmsg->reverse_stream == sid ? !p : (f =  dc_index_.find(pmsg->reverse_stream), 
                                p = f ==  dc_index_.end() ? nullptr : f->second, !p)) {
                            LOG(ERROR) << "channel " << sid << " has no match for open_resp " << std::endl;
                            //just ignore ...
                        }
                        else {
                             dcctrl_open_ack(p);
                        }
                    }
                        break;
                    case DATA_CHANNEL_ACK:
                        if (!p)
                        {
                            LOG(ERROR) << "channel " << sid << " has no match for open_ack " << std::endl;
                        }else
                        {
                             dcctrl_open_ack(p);
                        }
                        break;
                    default:
                        break;
                    }

                    break;
                case DATA_CHANNEL_PPID_DOMSTRING:
                case DATA_CHANNEL_PPID_PARTIAL_DOMSTRING:
                    if (!p)
                    {
                        LOG(ERROR) << "channel " << sid << " has no match for open_ack " << std::endl;
                    }else
                    {
                        dcctrl_open_ack(p);
                    }
                    break;
                case DATA_CHANNEL_PPID_PARTIAL_BINARY:
                case DATA_CHANNEL_PPID_BINARY:
                    if (!p)
                    {
                        LOG(ERROR) << "channel " << sid << " has no match for open_ack " << std::endl;
                    }else
                    {
                        dcctrl_open_ack(p);
                    }
                    break;
                case DATA_CHANNEL_PPID_EMPTY_STRING:
                case DATA_CHANNEL_PPID_EMPTY_BINARY:
                    break;
                default:
                    LOG(ERROR) << "Channel " << sid << "get unspecified ppid " << ppid << std::endl;
                    break;
                }

            }//end of lambda
            );

        }

        boost::asio::io_service&    io_srv_;

        void dcctrl_open(boost::shared_ptr<DataChannelImpl> p)
        {
            p->setSocket(reinterpret_cast<struct socket*>(bind_socket_->nativeHandle()));
        }

        void dcctrl_open_ack(boost::shared_ptr<DataChannelImpl>)
        {

        }

        void dcctrl_close(boost::shared_ptr<DataChannelImpl>)
        {

        }

    public:
        DataChannelCoreImpl(boost::asio::io_service& ios) : io_srv_(ios)
        {}
    };

}




}//namespace rtcdc