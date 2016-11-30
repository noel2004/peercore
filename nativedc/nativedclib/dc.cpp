#include "dc.h"
#include "usrsctplib/usrsctp.h"
#include "glog/logging.h"

namespace rtcdc
{

namespace {

class SCTPSocketCoreImpl : public SCTPSocketCore
{
    static int usrsctp_receive_cb(struct socket *sock, union sctp_sockstore addr, void *data,
        size_t datalen, struct sctp_rcvinfo rcv, int flags, void *ulp_info)
    {
        
    }

    struct socket*  usrsctp_sock_;
    bool            is_one_to_one_;

public:
    SCTPSocketCoreImpl(bool one_to_one) : 
        is_one_to_one_(one_to_one), usrsctp_sock_(nullptr){}

    bool    init(sctp::AssociationBase* base, unsigned short sctpport)
    {
        usrsctp_sock_ = usrsctp_socket(AF_CONN, is_one_to_one_ ? SOCK_STREAM : SOCK_SEQPACKET, 
            IPPROTO_SCTP, usrsctp_receive_cb, NULL, 0, this);
        if (usrsctp_sock_ == nullptr)
        {
            LOG(ERROR) << "create sctp socket failed: " << errno << std::endl;
            return false;
        }

        do
        {
            LOG(INFO) << "bind sctp socket to AF_CONN addr: "<<base->layerName()<<"@"<<sctpport<< std::endl;
            struct sockaddr_conn sconn = { AF_CONN, htons(sctpport), base };
            if (usrsctp_bind(usrsctp_sock_, (struct sockaddr *)&sconn, sizeof(struct sockaddr_conn)) < 0)
            {
                LOG(ERROR) << "bind fail: " << errno << std::endl;
                break;
            }

            //usrsctp_listen

        } while (false);

        usrsctp_close(usrsctp_sock_);
        usrsctp_sock_ = nullptr;
        return false;
    }

    ~SCTPSocketCoreImpl()
    {
        if(usrsctp_sock_ != nullptr){
            //for safety ...
            usrsctp_set_ulpinfo(usrsctp_sock_, nullptr);
            usrsctp_close(usrsctp_sock_);
        }
    }

    void*   nativeHandle() override{return usrsctp_sock_;}
    bool    oneToManyMode() const override{return !is_one_to_one_;}
//  TODO, for one-to-many mode:
//    boost::shared_ptr<SocketCore>   peeloff();
};



}



}//namespace rtcdc