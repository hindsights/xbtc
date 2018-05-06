#pragma once

#include "util/number.hpp"
#include <vector>
#include <stdint.h>

namespace xul {
class data_input_stream;
class data_output_stream;
}

namespace xbtc {


const int SERIALIZE_TRANSACTION_NO_WITNESS = 0x40000000;

class TransactionOutPoint
{
public:
    uint256 hash;
    uint32_t index;
    TransactionOutPoint() : index(-1) {}
    TransactionOutPoint(const uint256& h, uint32_t n) : hash(h), index(n) {}
};

inline bool operator==(const TransactionOutPoint& x, const TransactionOutPoint& y)
{
    return x.hash == y.hash && x.index == y.index;
}

class TransactionWitness
{
public:
    std::vector<std::vector<uint8_t> > stack;

    TransactionWitness() { }

    bool isNull() const { return stack.empty(); }
    void setNull() { stack.clear(); stack.shrink_to_fit(); }
};

class Script
{
public:
    std::vector<uint8_t> buffer;
};

class TransactionInput
{
public:
    TransactionOutPoint previousOutput;
    std::string signatureScript;
    TransactionWitness witness;
    uint32_t sequence;

    TransactionInput() : sequence(-1) {}
};

class TransactionOutput
{
public:
    int64_t value;
    std::string scriptPublicKey;

    TransactionOutput() : value(0) {}
};

class Transaction;
xul::data_input_stream& operator>>(xul::data_input_stream& is, Transaction& tx);
xul::data_output_stream& operator<<(xul::data_output_stream& os, const Transaction& tx);

xul::data_input_stream& operator>>(xul::data_input_stream& is, TransactionInput& input);
xul::data_output_stream& operator<<(xul::data_output_stream& os, const TransactionInput& input);

xul::data_input_stream& operator>>(xul::data_input_stream& is, TransactionOutput& output);
xul::data_output_stream& operator<<(xul::data_output_stream& os, const TransactionOutput& output);

xul::data_input_stream& operator>>(xul::data_input_stream& is, TransactionOutPoint& outpoint);
xul::data_output_stream& operator<<(xul::data_output_stream& os, const TransactionOutPoint& outpoint);

xul::data_input_stream& operator>>(xul::data_input_stream& is, TransactionWitness& witness);
xul::data_output_stream& operator<<(xul::data_output_stream& os, const TransactionWitness& witness);

class Transaction
{
public:
    int32_t version;
    std::vector<TransactionInput> inputs;
    std::vector<TransactionOutput> outputs;
    uint32_t lockTime;
    uint256 hash;

    Transaction() : version(0), lockTime(0)
    {
    }

    bool hasWitness() const;

    void computeHash();
    const uint256& getHash() const
    {
        assert(!hash.is_null());
        return hash;
    }

    bool isCoinBase() const { return inputs.size() == 1 && inputs[0].previousOutput.hash.is_null(); }
};


}
