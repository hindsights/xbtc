#pragma once

#include "Message.hpp"
#include "NodeAddress.hpp"
#include "protocols.hpp"
#include "data/Block.hpp"
#include "util/number.hpp"
#include <vector>
#include <stdint.h>

namespace xbtc {


class PrefilledTransaction
{
public:
    uint64_t index;
    Transaction transaction;
    PrefilledTransaction() : index(0) {}
};

class BlockHeaderAndShortTxIDs
{
public:
    BlockHeader header;
    uint64_t nonce;
    std::vector<UInt48> shortIDs;
    std::vector<PrefilledTransaction> prefilledTransactions;

    BlockHeaderAndShortTxIDs() : nonce(0) {}
};

class BlockTransactionsRequest
{
public:
    uint256 blockHash;
    std::vector<uint64_t> indexes;
};

class BlockTransactions
{
public:
    uint256 blockHash;
    std::vector<Transaction> transactions;
};


class VersionMessage : public Message
{
public:
    uint32_t version;
    uint64_t services;
    uint64_t timestamp;
    NodeAddress yourAddress;
    NodeAddress myAddress;
    uint64_t nonce;
    std::string userAgent;
    int32_t startHeight;
    bool relay;

    VersionMessage();
    virtual const char* getMessageType() const { return MSGTYPE_version; }
    virtual bool read_object(xul::data_input_stream& is);
    virtual void write_object(xul::data_output_stream& os) const;
};

class EmptyMessage : public Message
{
public:
    virtual bool read_object(xul::data_input_stream& is) { return true; }
    virtual void write_object(xul::data_output_stream& os) const {}
};

class VerackMessage : public EmptyMessage
{
public:
    virtual const char* getMessageType() const { return MSGTYPE_verack; }
};

class RejectMessage : public Message
{
public:
    std::string message;
    uint8_t code;
    std::string reason;
    std::string data;
    RejectMessage() : code(0) {}
    virtual const char* getMessageType() const { return MSGTYPE_reject; }
    virtual bool read_object(xul::data_input_stream& is);
    virtual void write_object(xul::data_output_stream& os) const;
};

class GetAddrMessage : public EmptyMessage
{
public:
    virtual const char* getMessageType() const { return MSGTYPE_getaddr; }
};

class SendHeadersMessage : public EmptyMessage
{
public:
    virtual const char* getMessageType() const { return MSGTYPE_sendheaders; }
};

class FeeFilterMessage : public EmptyMessage
{
public:
    int64_t feeRate;
    virtual bool read_object(xul::data_input_stream& is);
    virtual void write_object(xul::data_output_stream& os) const;
    virtual const char* getMessageType() const { return MSGTYPE_feefilter; }
};

class SendCmpctMessage : public EmptyMessage
{
public:
    bool announceUsingCMPCTBLOCK;
    uint64_t CMPCTBLOCKVersion;
    virtual bool read_object(xul::data_input_stream& is);
    virtual void write_object(xul::data_output_stream& os) const;
    virtual const char* getMessageType() const { return MSGTYPE_sendcmpct; }
};

class AddressInfo
{
public:
    uint32_t timestamp;
    NodeAddress address;
};

class AddrMessage : public Message
{
public:
    std::vector<AddressInfo> addresses;
    virtual const char* getMessageType() const { return MSGTYPE_addr; }
    virtual bool read_object(xul::data_input_stream& is);
    virtual void write_object(xul::data_output_stream& os) const;
};

class InventoryItem
{
public:
    uint32_t type;
    uint256 hash;

    InventoryItem() : type(0) { }
    explicit InventoryItem(uint32_t t, const uint256& h) : type(t), hash(h) { }
};

class InventoryMessageBase : public Message
{
public:
    std::vector<InventoryItem> inventory;
    virtual bool read_object(xul::data_input_stream& is);
    virtual void write_object(xul::data_output_stream& os) const;
};

class InvMessage : public InventoryMessageBase
{
public:
    virtual const char* getMessageType() const { return MSGTYPE_inv; }
};

class GetDataMessage : public InventoryMessageBase
{
public:
    virtual const char* getMessageType() const { return MSGTYPE_getdata; }
};

class NotFoundMessage : public InventoryMessageBase
{
public:
    virtual const char* getMessageType() const { return MSGTYPE_notfound; }
};

class BlockLocatorMessageBase : public Message
{
public:
    uint32_t version;
    std::vector<uint256> hashes;
    uint256 hashStop;
    virtual bool read_object(xul::data_input_stream& is);
    virtual void write_object(xul::data_output_stream& os) const;
};

class GetBlocksMessage : public BlockLocatorMessageBase
{
public:
    virtual const char* getMessageType() const { return MSGTYPE_getblocks; }
};

class GetHeadersMessage : public BlockLocatorMessageBase
{
public:
    virtual const char* getMessageType() const { return MSGTYPE_getheaders; }
};


class PingPongMessageBase : public Message
{
public:
    uint64_t nonce;
    virtual bool read_object(xul::data_input_stream& is);
    virtual void write_object(xul::data_output_stream& os) const;
};

class PingMessage : public PingPongMessageBase
{
public:
    virtual const char* getMessageType() const { return MSGTYPE_ping; }
};

class PongMessage : public PingPongMessageBase
{
public:
    virtual const char* getMessageType() const { return MSGTYPE_pong; }
};

class HeadersMessage : public Message
{
public:
    std::vector<BlockHeader> headers;
    virtual bool read_object(xul::data_input_stream& is);
    virtual void write_object(xul::data_output_stream& os) const;
    virtual const char* getMessageType() const { return MSGTYPE_headers; }
};

class BlockMessage : public Message
{
public:
    BlockPtr block;

    BlockMessage()
    {
        block = createBlock();
    }
    virtual bool read_object(xul::data_input_stream& is);
    virtual void write_object(xul::data_output_stream& os) const;
    virtual const char* getMessageType() const { return MSGTYPE_block; }
};

xul::data_input_stream& operator>>(xul::data_input_stream& is, AddressInfo& addr);
xul::data_output_stream& operator<<(xul::data_output_stream& os, const AddressInfo& addr);


}
