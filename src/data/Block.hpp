#pragma once

#include "Transaction.hpp"
#include "util/number.hpp"
#include <xul/io/serializable.hpp>
#include <xul/lang/object_ptr.hpp>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <stdint.h>
#include <assert.h>

namespace xul {
class data_input_stream;
class data_output_stream;
}

namespace xbtc {


enum BlockStatus : uint32_t
{
    //! Unused.
    BLOCK_VALID_UNKNOWN      =    0,

    //! Parsed, version ok, hash satisfies claimed PoW, 1 <= vtx count <= max, timestamp not in future
    BLOCK_VALID_HEADER       =    1,

    //! All parent headers found, difficulty matches, timestamp >= median previous, checkpoint. Implies all parents
    //! are also at least TREE.
    BLOCK_VALID_TREE         =    2,

    /**
     * Only first tx is coinbase, 2 <= coinbase input script length <= 100, transactions valid, no duplicate txids,
     * sigops, size, merkle root. Implies all parents are at least TREE but not necessarily TRANSACTIONS. When all
     * parent blocks also have TRANSACTIONS, CBlockIndex::nChainTx will be set.
     */
    BLOCK_VALID_TRANSACTIONS =    3,

    //! Outputs do not overspend inputs, no double spends, coinbase output ok, no immature coinbase spends, BIP30.
    //! Implies all parents are also at least CHAIN.
    BLOCK_VALID_CHAIN        =    4,

    //! Scripts & signatures ok. Implies all parents are also at least SCRIPTS.
    BLOCK_VALID_SCRIPTS      =    5,

    //! All validity bits.
    BLOCK_VALID_MASK         =   BLOCK_VALID_HEADER | BLOCK_VALID_TREE | BLOCK_VALID_TRANSACTIONS |
                                 BLOCK_VALID_CHAIN | BLOCK_VALID_SCRIPTS,

    BLOCK_HAVE_DATA          =    8, //!< full block available in blk*.dat
    BLOCK_HAVE_UNDO          =   16, //!< undo data available in rev*.dat
    BLOCK_HAVE_MASK          =   BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO,

    BLOCK_FAILED_VALID       =   32, //!< stage after last reached validness failed
    BLOCK_FAILED_CHILD       =   64, //!< descends from failed block
    BLOCK_FAILED_MASK        =   BLOCK_FAILED_VALID | BLOCK_FAILED_CHILD,

    BLOCK_OPT_WITNESS       =   128, //!< block data in blk*.data was received with a witness-enforcing client
};


class DiskBlockPos
{
public:
    int fileIndex;
    unsigned position;

    DiskBlockPos()
    {
        clear();
    }

    DiskBlockPos(int fileIn, unsigned pos)
    {
        fileIndex = fileIn;
        position = pos;
    }

    void clear()
    {
        fileIndex = -1;
        position = 0;
    }
    bool isNull() const
    {
        return fileIndex == -1;
    }
};

inline bool operator==(const DiskBlockPos &a, const DiskBlockPos &b)
{
    return (a.fileIndex == b.fileIndex && a.position == b.position);
}

inline bool operator!=(const DiskBlockPos &a, const DiskBlockPos &b)
{
    return !(a == b);
}

class BlockHeader
{
public:
    uint32_t version;
    uint256 previousBlockHash;
    uint256 merkleRootHash;
    uint32_t timestamp;
    uint32_t bits;
    uint32_t nonce;
    uint256 hash;

    BlockHeader()
    {
        doClear();
    }
    void clear()
    {
        doClear();
        previousBlockHash.clear();
        merkleRootHash.clear();
        hash.clear();
    }
    const uint256& computeHash();
private:
    void doClear()
    {
        version = 0;
        timestamp = 0;
        bits = 0;
        nonce = 0;
    }
};


class BlockFileInfo
{
public:
    int fileIndex;
    unsigned blocks;
    unsigned size;
    unsigned undoSize;
    unsigned minHeight;
    unsigned maxHeight;
    uint64_t minTime;
    uint64_t maxTime;

    BlockFileInfo()
    {
        clear();
    }
    void clear();
    void addBlock(unsigned height, uint64_t timestamp);
};

class BlockIndex : public xul::object, public xul::serializable
{
public:
    BlockHeader header;
    int height;
    uint32_t status;
    uint32_t transactionCount;
    int fileIndex;
    uint32_t dataPosition;
    uint32_t undoPosition;
    uint256 chainWork;
    uint32_t chainTransactionCount;
    int32_t sequenceId;
    uint32_t maxTime;
    boost::intrusive_ptr<BlockIndex> previous;
    boost::intrusive_ptr<BlockIndex> skip;

    BlockIndex()
    {
        doClear();
    }
    void clear()
    {
        header.clear();
        previous.reset();
        skip.reset();
        doClear();
    }
    void buildSkip();

    const uint256& getHash() const
    {
        assert(!header.hash.is_null());
        return header.hash;
    }
    void setDiskPosition(const DiskBlockPos& pos)
    {
        fileIndex = pos.fileIndex;
        dataPosition = pos.position + 8;
        undoPosition = 0;
    }

    BlockIndex* getAncestor(int destHeight);
    bool raiseValidity(enum BlockStatus upTo);
    bool isValid(enum BlockStatus upTo = BLOCK_VALID_TRANSACTIONS) const
    {
        assert(!(upTo & ~BLOCK_VALID_MASK)); // Only validity flags allowed.
        if (status & BLOCK_FAILED_MASK)
            return false;
        return ((status & BLOCK_VALID_MASK) >= upTo);
    }
    virtual bool read_object(xul::data_input_stream& is);
    virtual void write_object(xul::data_output_stream& os) const;
private:
    void doClear()
    {
        height = 0;
        status = 0;
        transactionCount = 0;
        fileIndex = 0;
        dataPosition = 0;
        undoPosition = 0;
        chainTransactionCount = 0;
        sequenceId = 0;
        maxTime = 0;
    }
};

class Block : public xul::object
{
public:
    BlockHeader header;
    std::vector<Transaction> transactions;

    BlockIndex* createBlockIndex();
    const uint256& getHash() const { return header.hash; }
};

typedef boost::intrusive_ptr<Block> BlockPtr;

typedef boost::intrusive_ptr<BlockIndex> BlockIndexPtr;
typedef std::unordered_map<uint256, BlockIndexPtr> BlockIndexMap;
typedef std::shared_ptr<BlockIndexMap> BlockIndexMapPtr;

class BlockIndexesData
{
public:
    BlockIndexMapPtr blocks;
    // std::vector<BlockIndexPtr> blocks;
    std::vector<BlockFileInfo> files;
    int lastBlockFile;

    BlockIndexesData() : blocks(std::make_shared<BlockIndexMap>())
    {
        lastBlockFile = 0;
    }
};


Block* createBlock();
BlockIndex* createBlockIndex();
BlockIndex* createBlockIndex(const BlockHeader& h);

xul::data_input_stream& operator>>(xul::data_input_stream& is, BlockHeader& header);
xul::data_output_stream& operator<<(xul::data_output_stream& os, const BlockHeader& header);

xul::data_input_stream& operator>>(xul::data_input_stream& is, BlockFileInfo& info);
xul::data_output_stream& operator<<(xul::data_output_stream& os, const BlockFileInfo& info);

xul::data_input_stream& operator>>(xul::data_input_stream& is, Block& block);
xul::data_output_stream& operator<<(xul::data_output_stream& os, const Block& block);


}
