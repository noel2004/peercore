#pragma once
#ifndef NATIVEDATACHANNEL_LIB_SCTP_BASE_H
#define NATIVEDATACHANNEL_LIB_SCTP_BASE_H

#include <boost/asio.hpp>
#include <boost/atomic.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>
#include <string>

namespace rtcdc
{
    class SCTP_tunnel
    {
    public:
        void    open(boost::function<void>(const boost::asio::mutable_buffer&),
            boost::function<void>(const std::string&),
            boost::system::error_code&);
        void    sendMessage(const boost::asio::mutable_buffer&);    //binary, blob
        void    sendMessage(const std::string&);                    //text
    };

    namespace sctp
    {
        class AssociationBase
        {
        protected:
            AssociationBase();
            //ecn bits was also a tos related field
            void     onRecvFromToLower(const unsigned char*, size_t, short ecn_bits);
        public:
            //tos indicate type of service (see https://en.wikipedia.org/wiki/Type_of_service)
            //and set_df indicate IP_DF should be set
            //both field works in IPv4 only
            virtual int     onDispatchToLower(const boost::asio::mutable_buffer&,
                short tos, short set_df ) = 0;

            virtual ~AssociationBase();
            
        };

        //our wrapper only allow a sending on message by message 
        class SocketBase
        {
        protected:
            //void    on

        public:
            virtual ~SocketBase();
        };

        class Module
        {
            static boost::atomic<Module*> instance_;
            static boost::mutex instantiation_mutex_;
        public:
            static Module* instance();
            //called in a thread-safe condition (mostly in main)
            static void Init(bool release = false);

            virtual ~Module();
       
        };
    }




}//namespace rtcdc

#endif //NATIVEDATACHANNEL_LIB_SCTP_BASE_H

