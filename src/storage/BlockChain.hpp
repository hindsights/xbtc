#pragma once

#include "util/number.hpp"
#include <xul/lang/object.hpp>
#include <vector>


namespace xbtc {


class BlockIndex;
class BlockHeader;
class Block;
class BlockStorage;
class AppConfig;
class ChainParams;

class BlockChain : public xul::object
{
public:
    virtual int getHeight() const = 0;
    virtual bool contains(const BlockIndex* block) const = 0;
    virtual BlockIndex* next(BlockIndex* block) const = 0;
    virtual BlockIndex* getTip() const = 0;
    virtual void setTip(BlockIndex* block) = 0;
    virtual BlockIndex* getBlock(int height) const = 0;
    virtual void getLocator(std::vector<uint256>& have, BlockIndex* block) const = 0;
};

BlockChain* createBlockChain();
BlockIndex* findLastCommonAncestor(BlockIndex* x, BlockIndex* y);

}
