#pragma once
#pragma once
#ifndef NATIVEDATACHANNEL_LIB_DATACHANNEL_TRANSPORT_INTERFACE_H
#define NATIVEDATACHANNEL_LIB_DATACHANNEL_TRANSPORT_INTERFACE_H

#include <string>
namespace boost { namespace asio { class mutable_buffer; } }

namespace rtcdc
{

class DatachannelCoreCall
{
public:
    virtual void    onAssocError(const std::string& reason, bool closed = false) = 0;
    virtual void    onAssocOpened(unsigned short streams[2]/*in/out*/) = 0;
    virtual void    onAssocClosed() = 0;//closed by remote peer
    //blob and text message ...
    //in blob entry the last parameter is used for increase performance when coordiante with usrsctp ...
    //with true the entry is responsed for free the buffer use the c-style free function ...
    virtual void    onDCMessage(unsigned short sid, unsigned int ppid, 
        const boost::asio::mutable_buffer&, void (*freebuffer)(void*) = nullptr) = 0;
    virtual void    onCanSendMore() = 0;
};


}

#endif //NATIVEDATACHANNEL_LIB_DATACHANNEL_TRANSPORT_INTERFACE_H
