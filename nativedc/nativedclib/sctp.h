#pragma once
#ifndef NATIVEDATACHANNEL_LIB_SCTP_BASE_H
#define NATIVEDATACHANNEL_LIB_SCTP_BASE_H

#include <boost/asio.hpp>
#include <boost/atomic.hpp>
#include <boost/thread/mutex.hpp>

namespace rtcdc
{
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

        //our wrapper for a one-to-one socket which only allows sending message by message 
        //the message sending may fail for EWOULDBLOCK (buffer is full)
        //but we have no way to notify when sending become availiable again yet ...
        //it was up to the caller to handle such a case 
        class SocketBase
        {
            void*   usrsctp_sock_;//c wrapper of usrsctp socket
        protected:
            //void    on
            virtual void onRecvv(unsigned short sid, const boost::asio::mutable_buffer&, unsigned int ppid) = 0;
            virtual void onRecvv(const boost::asio::mutable_buffer&) = 0;
            SocketBase(AssociationBase*);

        public:
            bool    isOK() const;
            virtual ~SocketBase();
            //simplified sending entries: only sid/ppid and retransmission, ordering options is specified
            int     sendv(unsigned short sid, const boost::asio::mutable_buffer&, unsigned int ppid, int rtx = 0, bool ord = false);
            //an empty sending without info
            int     sendv(const boost::asio::mutable_buffer&);

            //some flag testing entries ...
            bool    errorIsWouldBlock(int);
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

