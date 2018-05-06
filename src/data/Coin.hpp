#pragma once

#include "Transaction.hpp"
#include "util/number.hpp"
#include "util/hasher.hpp"
#include <xul/io/serializable.hpp>
#include <xul/lang/object_ptr.hpp>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <stdint.h>
#include <assert.h>

namespace xul {
class data_input_stream;
class data_output_stream;
}

namespace xbtc {


class Coin
{
public:
    TransactionOutput output;
    int height;
    bool isCoinBase;
    Coin() : height(0), isCoinBase(false) {}
    Coin(const TransactionOutput& out, int h, bool coinbase) : output(out), height(h), isCoinBase(coinbase) {}
};

xul::data_input_stream& operator>>(xul::data_input_stream& is, Coin& coin);
xul::data_output_stream& operator<<(xul::data_output_stream& os, const Coin& coin);

class CoinEntry
{
public:
    Coin coin;
    bool dirty;

    CoinEntry()
    {
        dirty = false;
    }
    explicit CoinEntry(const Coin& c, bool isdirty) : coin(c), dirty(isdirty)
    {
    }
};

class SipOutPointHasher
{
public:
    mutable RandomSipHasher hasher;
    size_t operator()(const TransactionOutPoint& outpoint) const
    {
        return hasher.hashUInt256WithExtra(outpoint.hash, outpoint.index);
    }
};

typedef std::unordered_map<TransactionOutPoint, CoinEntry, SipOutPointHasher> CoinMap;
typedef std::unordered_set<TransactionOutPoint, SipOutPointHasher> CoinSet;

class CoinsData
{
public:
    uint256 bestBlockHash;
    int bestBlockHeight;
    CoinMap addedCoins;
    CoinSet removedCoins;

    CoinsData()
    {
        bestBlockHeight = 0;
    }
};


}
