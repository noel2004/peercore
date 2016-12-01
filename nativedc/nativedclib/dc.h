#pragma once
#ifndef NATIVEDATACHANNEL_LIB_DATACHANNEL_H
#define NATIVEDATACHANNEL_LIB_DATACHANNEL_H

#include "sctp.h"
#include <boost/function.hpp>
#include <string>

namespace rtcdc
{
class DatachannelCoreCall;

class SCTPSocketCore
{
public:
    virtual ~SCTPSocketCore();

    static boost::shared_ptr<SCTPSocketCore>   createSocketCore(
        sctp::AssociationBase*, 
        unsigned short port, bool one_to_one = true);
    virtual void*   nativeHandle() = 0;
    virtual bool    oneToManyMode() const = 0;
    //socket for AF_CONN type can only distinguish associations by their ports
    //(all assoc's addrs are identify), port = 0 indicate remove a entry
    virtual bool    addDemultiplexEntry(unsigned short port, DatachannelCoreCall*) = 0;
    virtual bool    addEntry(unsigned short port) = 0;
//  TODO, for one-to-many mode:
//    boost::shared_ptr<SocketCore>   peeloff();
};

//the transport core for datachannel, which will bound to a
//one-to-one or one-to-many socket

class DataChannelCore
{
    boost::shared_ptr<SCTPSocketCore>   transport_;
public:
    virtual DatachannelCoreCall*   getDCCoreCallback() = 0;
    virtual sctp::AssociationBase* getAssociationBase() = 0;
};

//datachannel interface
class DataChannel
{
protected:
    unsigned short  sid_;
    DataChannel();
public:

    virtual ~DataChannel();
    //blob type
    int     sendMsg(const boost::asio::mutable_buffer&);
    //string type
    int     sendMsg(const std::string&);

    //some flag testing entries ...
    bool    errorIsWouldBlock(int);
};


}

#endif //NATIVEDATACHANNEL_LIB_DATACHANNEL_H
