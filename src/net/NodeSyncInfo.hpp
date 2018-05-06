#pragma once

#include "data/Block.hpp"
#include "util/number.hpp"

namespace xbtc {


class BlockCache;
class AppConfig;

class NodeSyncInfo
{
public:
    BlockIndexPtr bestKnownBlock;
    uint256 lastUnknownBlockHash;
    BlockIndexPtr lastCommonBlock;
    BlockIndexPtr lastDownloadBlock;
    BlockIndexMap requestingBlocks;

    void processBlockAvailability(BlockCache* cache);
    void updateBlockAvailability(const uint256& hash, BlockIndex* block, BlockCache* cache);
    void findBlocksToDownload(std::vector<BlockIndex*>& blocks, int count, BlockCache* cache, const AppConfig* config);
    bool isRequestingBlock() const { return !requestingBlocks.empty(); }
    void recordRequestingBlocks(const std::vector<BlockIndex*>& blocks);
    void removeBlockRecord(const Block* block);
};


}
