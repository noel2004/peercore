#pragma once
#ifndef NATIVEDATACHANNEL_LIB_IO_BASE_H
#define NATIVEDATACHANNEL_LIB_IO_BASE_H

#include <boost/asio.hpp>

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

    //additional wrapping for boost::asio::ip::udp::socket
    class asio_socketwrapper : public boost::asio::ip::udp::socket
    {
        bool    use_v4_;    //flag set when working under ipv4     
        bool    binded_;
        boost::asio::ip::udp::endpoint bind_ep_;

    public:
        asio_socketwrapper(boost::asio::io_service& ios) : 
            boost::asio::ip::udp::socket(ios), use_v4_(false), binded_(false){}
        ~asio_socketwrapper(){}

        typedef boost::asio::ip::udp::socket socket_type;

        void    open(boost::system::error_code& ec)
        {
            socket_type::open(socket_type::protocol_type::v6(), ec);
            if (!ec)
            {
                if (!net_stack_options::ipv6_only_stack_)
                {
                    //so we can listen on both v4 or v6 address
                    boost::asio::ip::v6_only opt(false);
                    socket_type::set_option(opt, ec);
                }
                if (!ec)return;
            }

            use_v4_ = true;
            socket_type::open(socket_type::protocol_type::v4(), ec);

        }

        //we adapt async_receive, and bind the dest ip automatically in first receive
        template<typename MutableBufferSequence, typename ReadHandler>
        void async_receive(const MutableBufferSequence & buffers, const ReadHandler &handler)
        {
            if (binded_) {
                boost::asio::ip::udp::socket::async_receive(buffers, handler);
            }
            else {
                boost::asio::ip::udp::socket::async_receive_from(
                    buffers, bind_ep_,
                    [this/*so we suppose the super class MUST refer us*/, 
                    handler](const boost::system::error_code& ec, std::size_t bytes_tran)
                    {
                        if (!ec)
                        {
                            boost::system::error_code ecc;
                            boost::asio::ip::udp::socket::connect(bind_ep_, ecc);
                            if (!ecc)binded_ = true;
                            else{
                                handler(ecc, bytes_tran);
                                return;
                            }
                        }
                        
                        handler(ec, bytes_tran);
                    }
                );
            }
        }

        void    prepare(boost::system::error_code& ec, unsigned short port = 0)
        {
            socket_type::bind(socket_type::endpoint_type(use_v4_ ?
                socket_type::protocol_type::v4() :
                socket_type::protocol_type::v6(), port), ec);

            if (ec)return;
        }

        unsigned short get_port(boost::system::error_code& ec)
        {
            auto ep = socket_type::local_endpoint(ec);
            if (!!ec)return 0;
            return ep.port();
        }

    };


}//namespace rtcdc

#endif
