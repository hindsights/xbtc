#pragma once

#include <xul/lang/object.hpp>
#include <stdint.h>


namespace xbtc {


class Message;
class NodeInfo;
class MessageEncoder;


class MessageSender : public xul::object
{
public:
    virtual bool sendMessage(const Message& msg) = 0;
};

MessageSender* createMessageSender(NodeInfo* nodeInfo, MessageEncoder* msgEncoder);


}
