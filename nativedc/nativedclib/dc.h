#pragma once
#ifndef NATIVEDATACHANNEL_LIB_DATACHANNEL_H
#define NATIVEDATACHANNEL_LIB_DATACHANNEL_H

#include "sctp.h"
#include <boost/function.hpp>
#include <string>

namespace boost { namespace asio { class io_service; } }

namespace rtcdc
{
class DatachannelCoreCall;

//the transport core for datachannel, which will bound to a
//one-to-one or one-to-many socket
class DataChannelCore
{
public:
    virtual DatachannelCoreCall*   getDCCoreCallback() = 0;
    virtual sctp::AssociationBase* getAssociationBase() = 0;
};

struct DataChannelOpt
{
    bool        ordered_;
    int         sid_;//-1 means not specified
    int         retrans_;//-1 means reliable
    std::string label_;
    std::string protocol_;
};

//datachannel interface
class DataChannel : public DataChannelOpt
{
protected:
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
