#pragma once
#ifndef NATIVEDATACHANNEL_LIB_DATACHANNEL_H
#define NATIVEDATACHANNEL_LIB_DATACHANNEL_H

#include "sctp.h"
#include <boost/function.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <string>

namespace rtcdc
{

class SCTPSocketCore : public boost::enable_shared_from_this<SCTPSocketCore>
{
public:
    virtual ~SCTPSocketCore();

    boost::shared_ptr<SCTPSocketCore>   createSocketCore(sctp::AssociationBase*, unsigned short port, 
        bool one_to_one = true);
    virtual void*   nativeHandle() = 0;
    virtual bool    oneToManyMode() const = 0;
//  TODO, for one-to-many mode:
//    boost::shared_ptr<SocketCore>   peeloff();
};

//the transport core for datachannel, which will bound to a
//one-to-one or one-to-many socket
class DataChannelCore
{
    boost::shared_ptr<SCTPSocketCore>   transport_;
public:
    DataChannelCore();
    DataChannelCore(SCTPSocketCore&);
    SCTPSocketCore& trans() const;
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
