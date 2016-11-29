#pragma once
#ifndef NATIVEDATACHANNEL_LIB_DATACHANNEL_H
#define NATIVEDATACHANNEL_LIB_DATACHANNEL_H

#include "sctp.h"
#include <boost/function.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <string>

namespace rtcdc
{


class SocketCore : public boost::enable_shared_from_this<SocketCore>
{
    const   bool is_one_to_one_;
    void*   usrsctp_sock_;//c wrapper of usrsctp socket
protected:
    //void    on
    virtual void onRecvv(unsigned short sid, const boost::asio::mutable_buffer&, unsigned int ppid) = 0;
    virtual void onRecvv(const boost::asio::mutable_buffer&) = 0;
    SocketCore(sctp::AssociationBase*);

public:
    bool    isOK() const;
    virtual ~SocketCore();

    int     bindTo(sctp::AssociationBase*);

};

//the transport core for datachannel, which will bound to a
//one-to-one or one-to-many socket
class DataChannelCore
{
    boost::shared_ptr<SocketCore>   transport_;
public:
    DataChannelCore();
    DataChannelCore(SocketCore&);
    SocketCore& trans() const;
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
