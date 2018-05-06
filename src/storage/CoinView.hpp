#pragma once

#include "util/number.hpp"
#include <xul/lang/object.hpp>


namespace xbtc {


class BlockIndex;
class BlockHeader;
class Block;
class AppConfig;
class ChainParams;
class BlockChain;
class Transaction;
class TransactionOutPoint;
class Coin;

class CoinView : public xul::object
{
public:
    virtual bool load() = 0;
    virtual const uint256& getBestBlockHash() const = 0;
    virtual void setBestBlockHash(const uint256& hash, int height) = 0;
    virtual void transfer(const Transaction* tx, int height) = 0;
    virtual bool hasCoin(const TransactionOutPoint& out) = 0;
    virtual Coin* fetchCoin(const TransactionOutPoint& out) = 0;
};

CoinView* createCoinView(const AppConfig* config);

}
