#pragma once

#include "Message.hpp"
#include <xul/lang/object.hpp>
#include <vector>
#include <stdint.h>


namespace xbtc {


class Message;

class MessageEncoder : public xul::object
{
public:
    virtual const uint8_t* getData() const = 0;
    virtual int encode(const Message& msg) = 0;
};

class MessageHolder : public xul::object
{
public:
    MessageHeader msg;
    std::vector<uint8_t> payload;
};

class MessageDecoderListener
{
public:
    virtual void onMessageDecoded(const MessageHeader& header, const std::vector<uint8_t>& payload) = 0;
};

class MessageDecoder : public xul::object
{
public:
    virtual void setListener(MessageDecoderListener* listener) = 0;
    virtual bool decode(const uint8_t* data, int size) = 0;
};


MessageEncoder* createMessageEncoder(uint32_t protocolMagic);
MessageDecoder* createMessageDecoder(uint32_t protocolMagic);


}
