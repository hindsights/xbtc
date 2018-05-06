#pragma once

#include <xul/io/serializable.hpp>
#include <string>
#include <stdint.h>


namespace xbtc {


const int PROTOCOL_HEADER_SIZE = 24;

class MessageHeader
{
public:
    uint32_t magic;
    std::string command;
    uint32_t length;
    uint32_t checksum;

    MessageHeader();
    virtual bool read_object(xul::data_input_stream& is);
    virtual void write_object(xul::data_output_stream& os) const;
};


class Message : public xul::serializable
{
public:
    virtual const char* getMessageType() const = 0;
    virtual ~Message() {}
};

}
