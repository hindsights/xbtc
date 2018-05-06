#pragma once

#include "util/number.hpp"
#include <xul/lang/object.hpp>


namespace xbtc {


class BlockIndex;
class BlockHeader;
class Block;
class BlockStorage;
class AppConfig;
class ChainParams;
class BlockChain;
class CoinView;

class BlockCache : public xul::object
{
public:
    virtual bool load() = 0;
    virtual BlockIndex* addBlock(Block* block) = 0;;
    virtual BlockIndex* addBlockIndex(const BlockHeader& header) = 0;
    virtual BlockIndex* getBlockIndex(const uint256& hash) = 0;
    virtual ChainParams* getChainParams() = 0;
    virtual void getLocator(std::vector<uint256>& have, const BlockIndex* block) const = 0;
    virtual BlockIndex* getTip() = 0;
    virtual BlockIndex* getBestHeader() = 0;
    virtual BlockChain* getChain() = 0;
    virtual CoinView* getCoinView() = 0;
};

BlockCache* createBlockCache(const AppConfig* config, BlockStorage* storage);

}
