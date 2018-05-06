#pragma once

#include "util/number.hpp"
#include <xul/lang/object.hpp>


namespace xbtc {


class BlockIndex;
class BlockHeader;
class Block;
class AppConfig;
class BlockFileInfo;
class BlockIndexesData;

class BlockIndexDB : public xul::object
{
public:
    virtual bool open() = 0;
    virtual void loadAll(BlockIndexesData& data) = 0;
    virtual bool writeBlocks(const BlockIndexesData& data) = 0;
    virtual bool readLastBlockFile(int& fileIndex) = 0;
    virtual bool readBlockFileInfo(int fileIndex, BlockFileInfo& info) = 0;
};

BlockIndexDB* createBlockIndexDB(const AppConfig* config);

}
