#pragma once

#include "util/number.hpp"
#include <xul/lang/object.hpp>


namespace xbtc {


class AppConfig;
class CoinsData;
class TransactionOutPoint;
class Coin;

class CoinDB : public xul::object
{
public:
    virtual bool open() = 0;
    virtual void loadAll(CoinsData& data) = 0;
    virtual bool readCoin(const TransactionOutPoint& out, Coin& coin) = 0;
    virtual bool writeCoins(const CoinsData& data) = 0;
};

CoinDB* createCoinDB(const AppConfig* config);

}
