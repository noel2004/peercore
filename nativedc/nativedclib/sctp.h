#pragma once
#ifndef NATIVEDATACHANNEL_LIB_SCTPWRAPPER_H
#define NATIVEDATACHANNEL_LIB_SCTPWRAPPER_H

#include <boost/asio.hpp>
#include <boost/atomic.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace rtcdc
{

    class SCTP_base : public boost::enable_shared_from_this<SCTP_base>
    {        
    protected:
        virtual void onDispatchToLower(const boost::asio::mutable_buffer&) = 0;
//        virtual void onDispatchToLower(const boost::asio::mutable_buffer&) = 0;
    };

    class SCTPModule
    {
        static boost::atomic<SCTPModule*> instance_;
        static boost::mutex instantiation_mutex_;
    public:
        static SCTPModule* instance();
        //called in a thread-safe condition (mostly in main)
        static void SCTPInit(bool release = false);

        virtual ~SCTPModule();
        //reg to port 0 will unreg the exist SCTP_base, and is always succeed
        virtual bool regSCTPLocalPort(int port, SCTP_base*) = 0;
    };

    //one traffic core act as an addr in AF_CONN mode of usrsctp
    template<class Layer>
    class SCTP_TrafficCore
    {
        Layer lowLevelTransLayer_;

        void onLowLayerIO(bool read, const boost::system::error_code&, std::size_t);

    public:
        SCTP_TrafficCore(boost::asio::io_service& ios) : lowLevelTransLayer_(ios){}
        void    open(boost::system::error_code&);
    };



}//namespace rtcdc

#endif //NATIVEDATACHANNEL_LIB_SCTPWRAPPER_H

