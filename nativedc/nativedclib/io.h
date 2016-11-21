#pragma once
#ifndef NATIVEDATACHANNEL_LIB_H
#define NATIVEDATACHANNEL_LIB_H

#include <boost/asio.hpp>

namespace rtcdc
{
    template<class Layer>
    class coroutineIO_t : public Layer
    {
        boost::asio::coroutine co_;
    public:
        coroutineIO_t(boost::asio::io_service& ios);
    };



}//namespace rtcdc

#endif
