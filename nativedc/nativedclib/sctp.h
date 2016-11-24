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
        //called in a thread-safe condition (mostly in main)
        static void SCTPInit(bool release = false);

        SCTPModule();
        ~SCTPModule();
    };

    class SCTP_base
    {
    public:
        virtual void onDispatchToLower(const boost::asio::mutable_buffer&) = 0;
    };

    template<class Layer>
    class SCTPOutbound : SCTP_base, boost::noncopyable
    {
        Layer lowLevelTransLayer_;

        //interface
        void onDispatchToLower(const boost::asio::mutable_buffer&) override;
    public:
        SCTPOutbound(boost::asio::io_service& ios) : lowLevelTransLayer_(ios){}
    };



}//namespace rtcdc

#endif //NATIVEDATACHANNEL_LIB_SCTPWRAPPER_H

