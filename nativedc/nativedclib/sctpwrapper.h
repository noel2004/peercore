#pragma once
#ifndef NATIVEDATACHANNEL_LIB_SCTP_WRAPPER_H
#define NATIVEDATACHANNEL_LIB_SCTP_WRAPPER_H

#include "sctp.h"
#include "io.h"
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>

namespace rtcdc
{

    template<class Layer>
    class SCTP_Association : sctp::AssociationBase, 
        public boost::enable_shared_from_this<SCTP_Association<Layer> >
    {
        Layer   &lowLevelTransLayer_;
        boost::array<unsigned char, net_stack_options::max_udp_mtu_size_> readbuffer_;
        bool    runFlag_;

        int     onDispatchToLower(const boost::asio::mutable_buffer& buf, short, short) override
        {
            //here we use SYNC api ...
            boost::system::error_code ec;
            lowLevelTransLayer_.send(buf, 0, ec);
            return !ec ? 0 : ec.value();
        }

        void    readMore()
        {
            if (!runFlag_)return;
            lowLevelTransLayer_.async_receive(boost::asio::buffer(readbuffer_),
                boost::bind<void>(
                    &SCTP_Association<Layer>::onLowLayelLayerIO,
                    shared_from_this(), 
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred
                    )
            );
        }

        void    onLowLayelLayerIO(const boost::system::error_code& error, std::size_t bytes_transferred)
        {
            if (error)return;
            sctp::AssociationBase::onRecvFromToLower(readbuffer_.data(), bytes_transferred, 0);
            readMore();
        }

    public:
        SCTP_Association(Layer& l) : lowLevelTransLayer_(l), runFlag_(false){}
        ~SCTP_Association(){}

        const char*     layerName() override
        {
            static const char n[] = "SCTP Association basic layer";
            return n;
        }

        void    init(boost::system::error_code& ec)
        {
            runFlag_ = true;
            readMore();
        }

        void    stop()
        {
            boost::system::error_code ec;
            lowLevelTransLayer_.cancel(ec);
            runFlag_ = false;
        }

    };




}//namespace rtcdc

#endif //NATIVEDATACHANNEL_LIB_SCTP_WRAPPER_H

#pragma once
