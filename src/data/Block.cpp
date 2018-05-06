#include "Block.hpp"
#include "version.hpp"
#include "util/serialization.hpp"
#include "util/Hasher.hpp"

#include <xul/data/big_number_io.hpp>
#include <xul/lang/object_impl.hpp>
#include <xul/macro/minmax.hpp>

namespace xbtc {


void BlockFileInfo::clear()
{
    fileIndex = 0;
    blocks = 0;
    size = 0;
    undoSize = 0;
    minHeight = 0;
    maxHeight = 0;
    minTime = 0;
    maxTime = 0;
}

void BlockFileInfo::addBlock(unsigned height, uint64_t timestamp)
{
    if (blocks == 0)
    {
        assert(minHeight == 0 && maxHeight == 0 && minTime == 0 && maxTime == 0);
        minHeight = maxHeight = height;
        minTime = maxTime = timestamp;
    }
    else
    {
        XUL_LIMIT_MAX(minHeight, height);
        XUL_LIMIT_MIN(maxHeight, height);
        XUL_LIMIT_MAX(minTime, timestamp);
        XUL_LIMIT_MIN(maxTime, timestamp);
    }
    blocks++;
}

const uint256& BlockHeader::computeHash()
{
    hash = Hasher256::hash_data(*this);
    return hash;
}

BlockIndex* Block::createBlockIndex()
{
    BlockIndex* bi = xbtc::createBlockIndex();
    bi->header = header;
    return bi;
}

Block* createBlock()
{
    return xul::create_object<Block>();
}

bool BlockIndex::read_object(xul::data_input_stream& is)
{
    int protoVersion = CLIENT_VERSION;
    is >> makeVarReader(protoVersion) >> makeVarReader(height) >> makeVarReader(status) >> makeVarReader(transactionCount);
    if (status & (BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO))
        is >> makeVarReader(fileIndex);
    if (status & BLOCK_HAVE_DATA)
        is >> makeVarReader(dataPosition);
    if (status & BLOCK_HAVE_UNDO)
        is >> makeVarReader(undoPosition);
    is >> header;
    return is.good();
}

void BlockIndex::write_object(xul::data_output_stream& os) const
{
    os << makeVarWriter(CLIENT_VERSION) << makeVarWriter(height) << makeVarWriter(status) << makeVarWriter(transactionCount);
    if (status & (BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO))
        os << makeVarWriter(fileIndex);
    if (status & BLOCK_HAVE_DATA)
        os << makeVarWriter(dataPosition);
    if (status & BLOCK_HAVE_UNDO)
        os << makeVarWriter(undoPosition);
    os << header;
}

inline int invertLowestOne(int n)
{
    return n & (n - 1);
}

inline int getSkipHeight(int height)
{
    if (height < 2)
        return 0;

    // Determine which height to jump back to. Any number strictly lower than height is acceptable,
    // but the following expression seems to perform well in simulations (max 110 steps to go back
    // up to 2**18 blocks).
    return (height & 1) ? invertLowestOne(invertLowestOne(height - 1)) + 1 : invertLowestOne(height);
}

void BlockIndex::buildSkip()
{
    if (previous)
        skip = previous->getAncestor(getSkipHeight(height));
}

BlockIndex* BlockIndex::getAncestor(int destHeight)
{
    if (destHeight > height || destHeight < 0)
    {
        return nullptr;
    }

    BlockIndex* pindexWalk = this;
    int heightWalk = height;
    while (heightWalk > destHeight)
    {
        int heightSkip = getSkipHeight(heightWalk);
        int heightSkipPrev = getSkipHeight(heightWalk - 1);
        if (pindexWalk->skip &&
            (heightSkip == destHeight ||
             (heightSkip > destHeight && !(heightSkipPrev < heightSkip - 2 &&
                                       heightSkipPrev >= destHeight)))) {
            // Only follow pskip if pprev->pskip isn't better than pskip->pprev.
            pindexWalk = pindexWalk->skip.get();
            heightWalk = heightSkip;
        }
        else
        {
            assert(pindexWalk->previous);
            pindexWalk = pindexWalk->previous.get();
            heightWalk--;
        }
    }
    return pindexWalk;
}

bool BlockIndex::raiseValidity(enum BlockStatus upTo)
{
    assert(!(upTo & ~BLOCK_VALID_MASK));
    if (status & BLOCK_FAILED_MASK)
        return false;
    if ((status & BLOCK_VALID_MASK) < upTo)
    {
        status = (status & ~BLOCK_VALID_MASK) | upTo;
        return true;
    }
    return false;
}

BlockIndex* createBlockIndex()
{
    return xul::create_object<BlockIndex>();
}

BlockIndex* createBlockIndex(const BlockHeader& h)
{
    auto b = xul::create_object<BlockIndex>();
    b->header = h;
    return b;
}

xul::data_input_stream& operator>>(xul::data_input_stream& is, BlockHeader& header)
{
    is >> header.version >> header.previousBlockHash >> header.merkleRootHash;
    return is >> header.timestamp >> header.bits >> header.nonce;
}

xul::data_output_stream& operator<<(xul::data_output_stream& os, const BlockHeader& header)
{
    os << header.version << header.previousBlockHash << header.merkleRootHash;
    return os << header.timestamp << header.bits << header.nonce;
}

xul::data_input_stream& operator>>(xul::data_input_stream& is, Block& block)
{
    return is >> block.header >> makeVarReader(block.transactions, 2000);
}

xul::data_output_stream& operator<<(xul::data_output_stream& os, const Block& block)
{
    return os << block.header << makeVarWriter(block.transactions);
}


xul::data_input_stream& operator>>(xul::data_input_stream& is, BlockFileInfo& info)
{
    is >> makeVarReader(info.blocks) >> makeVarReader(info.size) >> makeVarReader(info.undoSize);
    is >> makeVarReader(info.minHeight) >> makeVarReader(info.maxHeight);
    return is >> makeVarReader(info.minTime) >> makeVarReader(info.maxTime);
}

xul::data_output_stream& operator<<(xul::data_output_stream& os, const BlockFileInfo& info)
{
    os << makeVarWriter(info.blocks) << makeVarWriter(info.size) << makeVarWriter(info.undoSize);
    os << makeVarWriter(info.minHeight) << makeVarWriter(info.maxHeight);
    return os << makeVarWriter(info.minTime) << makeVarWriter(info.maxTime);
}


}
