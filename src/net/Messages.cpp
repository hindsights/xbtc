#include "Messages.hpp"
#include "util/serialization.hpp"

#include <xul/io/serializable.hpp>
#include <xul/io/data_input_stream.hpp>
#include <xul/io/data_output_stream.hpp>
#include <xul/net/sockets.hpp>
#include <xul/data/big_number_io.hpp>
#include <xul/data/bit_converter.hpp>

#include <assert.h>


namespace xbtc {


xul::data_input_stream& operator>>(xul::data_input_stream& is, NodeAddress& addr)
{
    is >> addr.services;
    uint8_t buf[12];
    is.read_bytes(buf, 12);
    is >> addr.address.ip >> addr.address.port;
    addr.address.ip = ntohl(addr.address.ip);
    addr.address.port = ntohs(addr.address.port);
    return is;
}

xul::data_output_stream& operator<<(xul::data_output_stream& os, const NodeAddress& addr)
{
    os.write_uint64(addr.services);
    uint8_t buf[12];
    memset(buf, 0, 12);
    if (addr.address.ip != 0)
    {
        buf[10] = 0xFF;
        buf[11] = 0xFF;
    }
    os.write_bytes(buf, 12);
    os.write_uint32(htonl(addr.address.ip));
    os.write_uint16(htons(addr.address.port));
    return os;
}


xul::data_input_stream& operator>>(xul::data_input_stream& is, AddressInfo& info)
{
    return is >> info.timestamp >> info.address;
}

xul::data_output_stream& operator<<(xul::data_output_stream& os, const AddressInfo& info)
{
    return os << info.timestamp << info.address;
}

xul::data_input_stream& operator>>(xul::data_input_stream& is, InventoryItem& item)
{
    return is >> item.type >> item.hash;
}

xul::data_output_stream& operator<<(xul::data_output_stream& os, const InventoryItem& item)
{
    return os << item.type << item.hash;
}

MessageHeader::MessageHeader()
{

}

bool MessageHeader::read_object(xul::data_input_stream& is)
{
    is >> magic;
    char buf[13];
    memset(buf, 0, 13);
    is.read_array(buf, 12);
    is >> length >> checksum;
    if (!is)
        return false;
    command.assign(buf);
    return true;
}

void MessageHeader::write_object(xul::data_output_stream& os) const
{
    assert(false);
}


VersionMessage::VersionMessage()
{
    version = 0;
    services = 0;
    timestamp = 0;
    nonce = 0;
    startHeight = 0;
    relay = false;
}
bool VersionMessage::read_object(xul::data_input_stream& is)
{
    is >> version >> services >> timestamp >> yourAddress;
    if (is.available() > 0)
    {
        is >> myAddress >> nonce;
    }
    if (is.available() > 0)
    {
        is >> LimitStringReader(userAgent, 128);
    }
    if (is.available() > 0)
    {
        is >> startHeight >> relay;
    }
    return is.good();
}
void VersionMessage::write_object(xul::data_output_stream& os) const
{
    os << version << services << timestamp << yourAddress << myAddress << nonce;
    os << LimitStringWriter(userAgent) << startHeight << relay;
}

bool RejectMessage::read_object(xul::data_input_stream& is)
{
    is >> LimitStringReader(message, 64) >> code >> LimitStringReader(reason, 1024);
    is.read_string(data, is.available());
    return is.good();
}
void RejectMessage::write_object(xul::data_output_stream& os) const
{
    os << LimitStringWriter(message) << code << LimitStringWriter(reason) << data;
}


bool AddrMessage::read_object(xul::data_input_stream& is)
{
    return VarEncoding::readVector(is, addresses, 5000);
}
void AddrMessage::write_object(xul::data_output_stream& os) const
{
    VarEncoding::writeVector(os, addresses);
}

bool InventoryMessageBase::read_object(xul::data_input_stream& is)
{
    return VarEncoding::readVector(is, inventory, 50000);
}
void InventoryMessageBase::write_object(xul::data_output_stream& os) const
{
    VarEncoding::writeVector(os, inventory);
}

bool BlockLocatorMessageBase::read_object(xul::data_input_stream& is)
{
    is >> version >> makeVarReader(hashes, 1000) >> hashStop;
    return is.good();
}
void BlockLocatorMessageBase::write_object(xul::data_output_stream& os) const
{
    os << version << makeVarWriter(hashes) << hashStop;
}

bool FeeFilterMessage::read_object(xul::data_input_stream& is)
{
    is >> feeRate;
    return is.good();
}
void FeeFilterMessage::write_object(xul::data_output_stream& os) const
{
    os << feeRate;
}

bool PingPongMessageBase::read_object(xul::data_input_stream& is)
{
    is >> nonce;
    return is.good();
}
void PingPongMessageBase::write_object(xul::data_output_stream& os) const
{
    os << nonce;
}

bool SendCmpctMessage::read_object(xul::data_input_stream& is)
{
    is >> announceUsingCMPCTBLOCK >> CMPCTBLOCKVersion;
    return is.good();
}
void SendCmpctMessage::write_object(xul::data_output_stream& os) const
{
    os << announceUsingCMPCTBLOCK << CMPCTBLOCKVersion;
}

bool BlockMessage::read_object(xul::data_input_stream& is)
{
    is >> *block;
    return is.good();
}
void BlockMessage::write_object(xul::data_output_stream& os) const
{
    os << *block;
}

bool HeadersMessage::read_object(xul::data_input_stream& is)
{
    uint64_t count = 0;
    if (!VarEncoding::readCompactSize(is, count))
        return false;
    if (count > 10000)
    {
        is.set_bad();
        return false;
    }
    headers.resize(count);
    for (auto& header : headers)
    {
        is >> header;
        uint64_t txcount = 0;
        if (!VarEncoding::readCompactSize(is, txcount))
            break;
        if (txcount != 0)
        {
            is.set_bad();
            break;
        }
    }
    return is.good();
}
void HeadersMessage::write_object(xul::data_output_stream& os) const
{
    VarEncoding::writeCompactSize(os, headers.size());
    for (const auto& header : headers)
    {
        os << header;
        os.write_uint8(0);
    }
}


}
