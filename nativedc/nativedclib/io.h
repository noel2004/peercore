#pragma once
#ifndef NATIVEDATACHANNEL_LIB_IO_BASE_H
#define NATIVEDATACHANNEL_LIB_IO_BASE_H

namespace rtcdc
{
    struct net_stack_options
    {
        //constants
        enum {
            max_udp_mtu_size_ = 2048,
        };

        static bool         ipv6_only_stack_;
    };

    //one traffic core act as an addr in AF_CONN mode of usrsctp
    template<class Layer>
    class SCTP_Association : AssociationBase
    {
        Layer   lowLevelTransLayer_;
        bool    use_v4_;    //flag set when working under ipv4

        int     onDispatchToLower(const boost::asio::mutable_buffer& buf, short, short) override
        {
            //here we use SYNC api ...
            boost::system::error_code ec;
            lowLevelTransLayer_.send(buf, 0, ec);
            return !ec ? 0 : ec.value();
        }        

    public:
        SCTP_TrafficCore(boost::asio::io_service& ios) : lowLevelTransLayer_(ios), use_v4_(false){}
        ~SCTP_TrafficCore()
        {

        }

        typedef typename lowLevelTransLayer_::endpoint_type endpoint_type;

        void    open(boost::system::error_code& ec)
        {
            lowLevelTransLayer_.open(lowLevelTransLayer_::protocol_type::v6(), ec);
            if (!ec)
            {
                if (!net_stack_options::ipv6_only_stack)
                {
                    //so we can listen on both v4 or v6 address
                    boost::asio::ip::v6_only opt(false);
                    lowLevelTransLayer_.set_option(opt, ec);
                }
                if (!ec)return;
            }

            use_v4_ = true;
            lowLevelTransLayer_.open(lowLevelTransLayer_::protocol_type::v4(), ec);

        }

        void    prepare(boost::system::error_code& ec, unsigned short port = 0)
        {
            lowLevelTransLayer_.bind(endpoint_type(use_v4_ ?
                lowLevelTransLayer_::protocol_type::v4() :
                lowLevelTransLayer_::protocol_type::v6(), port), ec);

            if (ec)return;


        }

        //TODO: an equivalent for socket::connect
        void    establish(const endpoint_type& ep){}

    };


}//namespace rtcdc

#endif
