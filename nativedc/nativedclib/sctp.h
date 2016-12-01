#pragma once
#ifndef NATIVEDATACHANNEL_LIB_SCTP_BASE_H
#define NATIVEDATACHANNEL_LIB_SCTP_BASE_H

#include <boost/shared_ptr.hpp>
namespace boost { namespace asio { class mutable_buffer; } }

namespace rtcdc
{
    class DatachannelCoreCall;

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
            virtual const char*     layerName() const = 0;

            virtual ~AssociationBase();

        };

        class Module
        {
        public:
            static Module* instance();
            //called in a thread-safe condition (mostly in main)
            static void Init(bool release = false);

            virtual ~Module();
       
        };

        //a socket wrapper built from an AF_CONN address (AssociationBase*) and
        //understand the mechanism in datachannel (i.e. what a dc need from the
        //received SCTP message and its info), it can also work under one-to-many
        //mode
        class SocketCore
        {
        public:
            virtual ~SocketCore();
            virtual void*   nativeHandle() = 0;
            virtual bool    oneToManyMode() const = 0;
            //socket for AF_CONN type can only distinguish associations by their ports
            //(all assoc's addrs are identify), port = 0 indicate remove a entry
            //if socket is under one-to-one mode, only one DCCall can be added
            virtual bool    addEntry(unsigned short port, DatachannelCoreCall*) = 0;
            //for one-to-many mode:
            virtual boost::shared_ptr<SocketCore>   peeloff() = 0;

            static boost::shared_ptr<SocketCore>   createSocketCore(
                AssociationBase*, unsigned short port, bool one_to_one = true);
        };
    }




}//namespace rtcdc

#endif //NATIVEDATACHANNEL_LIB_SCTP_BASE_H

