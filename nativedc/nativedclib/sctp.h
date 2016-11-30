#pragma once
#ifndef NATIVEDATACHANNEL_LIB_SCTP_BASE_H
#define NATIVEDATACHANNEL_LIB_SCTP_BASE_H

namespace boost { namespace asio { class mutable_buffer; } }

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
    }




}//namespace rtcdc

#endif //NATIVEDATACHANNEL_LIB_SCTP_BASE_H

