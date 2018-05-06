#include "Transaction.hpp"
#include "util/serialization.hpp"
#include "util/Hasher.hpp"

#include <xul/data/big_number_io.hpp>


namespace xbtc {


inline bool isWitnessAllowed(uint32_t version)
{
    return !(version & SERIALIZE_TRANSACTION_NO_WITNESS);
}

void Transaction::computeHash()
{
    hash = Hasher256::hash_data(*this);
}

bool Transaction::hasWitness() const
{
    for (const auto& in : inputs)
    {
        if (!in.witness.isNull())
        {
            return true;
        }
    }
    return false;
}

xul::data_input_stream& operator>>(xul::data_input_stream& is, Transaction& tx)
{
    const bool allowWitness = isWitnessAllowed(tx.version);
    is >> tx.version;
    bool needWitness = false;
    tx.inputs.clear();
    is >> makeVarReader(tx.inputs, 10000);
    if (allowWitness && tx.inputs.empty())
    {
        uint8_t flag = 0;
        if (!is.read_byte(flag))
            return is;
        if (flag != 0)
        {
            is >> makeVarReader(tx.inputs, 10000);
        }
    }
    is >> makeVarReader(tx.outputs, 10000);
    if (needWitness)
    {
        for (auto& in : tx.inputs)
        {
            is >> in.witness;
        }
    }
    return is >> tx.lockTime;
}

xul::data_output_stream& operator<<(xul::data_output_stream& os, const Transaction& tx)
{
    const bool allowWitness = isWitnessAllowed(tx.version);
    os << tx.version;
    bool needWitness = false;
    if (allowWitness && tx.hasWitness())
    {
        needWitness = true;
        os.write_uint16(0x0100);
    }
    os << makeVarWriter(tx.inputs) << makeVarWriter(tx.outputs);
    if (needWitness)
    {
        for (const auto& in : tx.inputs)
        {
            os << in.witness;
        }
    }
    return os << tx.lockTime;
}

xul::data_input_stream& operator>>(xul::data_input_stream& is, TransactionInput& input)
{
    return is >> input.previousOutput >> LimitStringReader(input.signatureScript, 50000) >> input.sequence;
}

xul::data_output_stream& operator<<(xul::data_output_stream& os, const TransactionInput& input)
{
    return os << input.previousOutput << LimitStringWriter(input.signatureScript) << input.sequence;
}

xul::data_input_stream& operator>>(xul::data_input_stream& is, TransactionOutput& output)
{
    return is >> output.value >> LimitStringReader(output.scriptPublicKey, 50000);
}
xul::data_output_stream& operator<<(xul::data_output_stream& os, const TransactionOutput& output)
{
    return os << output.value << LimitStringWriter(output.scriptPublicKey);
}

xul::data_input_stream& operator>>(xul::data_input_stream& is, TransactionOutPoint& outpoint)
{
    return is >> outpoint.hash >> outpoint.index;
}
xul::data_output_stream& operator<<(xul::data_output_stream& os, const TransactionOutPoint& outpoint)
{
    return os << outpoint.hash << outpoint.index;
}

xul::data_input_stream& operator>>(xul::data_input_stream& is, TransactionWitness& witness)
{
    uint64_t count = 0;
    if (!VarEncoding::readCompactSize(is, count))
        return is;
    if (count > 10000)
    {
        is.set_bad();
        return is;
    }
    witness.stack.resize(count);
    for (int i = 0; i < static_cast<int>(count); ++i)
    {
        VarEncoding::readVector(is, witness.stack[i], 10000);
    }
    return is;
}
xul::data_output_stream& operator<<(xul::data_output_stream& os, const TransactionWitness& witness)
{
    VarEncoding::writeCompactSize(os, witness.stack.size());
    for (const auto v : witness.stack)
    {
        VarEncoding::writeVector(os, v);
    }
    return os;
}


}
