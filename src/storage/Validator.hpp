#pragma once

#include <xul/lang/object.hpp>


namespace xbtc {


class BlockIndex;
class BlockHeader;
class Block;
class AppConfig;
class Transaction;
class CoinView;

class Validator : public xul::object
{
public:
    virtual bool validateBlockHeader(const BlockHeader& header) = 0;
    virtual bool validateBlockIndex(const BlockIndex* block) = 0;
    virtual bool validateBlock(const Block* block, const BlockIndex* blockIndex) = 0;
    virtual bool verifyTransactions(const Block* block, const BlockIndex* blockIndex) = 0;
};

Validator* createValidator(CoinView* coinView, const AppConfig* config);

}
