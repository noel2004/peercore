#pragma once
#ifndef NATIVEDATACHANNEL_LIB_SCTPWRAPPER_H
#define NATIVEDATACHANNEL_LIB_SCTPWRAPPER_H

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/atomic.hpp>
#include <boost/thread/mutex.hpp>

namespace rtcdc
{
    class SCTPModule
    {
        static boost::atomic<SCTPModule*> instance_;
        static boost::mutex instantiation_mutex;
    public:
        static SCTPModule* instance();
        //called in the thread-safe condition
        static void SCTPInit(bool release = false);

        SCTPModule();
        ~SCTPModule();
    };

    template<class Layer>
    class SCTPOutbound : boost::noncopyable
    {
        Layer lowLevelTransLayer_;
    public:
        SCTPOutbound(boost::asio::io_service& ios);
    };



}//namespace rtcdc

#endif //NATIVEDATACHANNEL_LIB_SCTPWRAPPER_H

