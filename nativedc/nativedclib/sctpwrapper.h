#pragma once
#ifndef NATIVEDATACHANNEL_LIB_SCTP_WRAPPER_H
#define NATIVEDATACHANNEL_LIB_SCTP_WRAPPER_H

#include "sctp.h"

namespace rtcdc
{

    //one traffic core act as an addr in AF_CONN mode of usrsctp
    template<class Layer>
    class SCTP_Association : AssociationBase
    {
        Layer   lowLevelTransLayer_;

        int     onDispatchToLower(const boost::asio::mutable_buffer& buf, short, short) override
        {
            boost::system::error_code ec;
            lowLevelTransLayer_.send(buf, 0, ec);
            return !ec ? 0 : ec.value();
        }

    public:
        SCTP_TrafficCore(boost::asio::io_service& ios) : lowLevelTransLayer_(ios){}
        ~SCTP_TrafficCore()
        {

        }

        void    open(boost::system::error_code& ec)
        {
            lowLevelTransLayer_.open(lowLevelTransLayer_::protocol_type(), ec);
            typename lowLevelTransLayer_::endpoint_type ep;

        }


    };



}//namespace rtcdc

#endif //NATIVEDATACHANNEL_LIB_SCTP_WRAPPER_H

#pragma once
