#include "BlockChain.hpp"
#include "data/Block.hpp"

#include <xul/lang/object_impl.hpp>
#include <xul/data/big_number_io.hpp>
#include <xul/data/date_time.hpp>
#include <xul/util/time_counter.hpp>
#include <xul/log/log.hpp>
#include <vector>
#include <deque>
#include <unordered_map>
#include <map>


namespace xbtc {


class BlockChainImpl : public xul::object_impl<BlockChain>
{
public:
    explicit BlockChainImpl()
    {
        XUL_LOGGER_INIT("BlockChain");
        XUL_REL_EVENT("new");
    }
    ~BlockChainImpl()
    {
        XUL_REL_EVENT("delete");
    }

    virtual int getHeight() const
    {
        assert(m_blocks.size() >= 1);
        return m_blocks.size() - 1;
    }
    virtual bool contains(const BlockIndex* block) const
    {
        return getBlock(block->height) == block;
    }
    virtual BlockIndex* next(BlockIndex* block) const
    {
        return getBlock(block->height + 1);
    }
    virtual BlockIndex* getTip() const
    {
        assert(!m_blocks.empty());
        return m_blocks[m_blocks.size() - 1].get();
    }
    virtual void setTip(BlockIndex* block)
    {
        if (block == nullptr)
        {
            m_blocks.clear();
            return;
        }
        m_blocks.resize(block->height + 1);
        while (block && m_blocks[block->height].get() != block)
        {
            m_blocks[block->height] = block;
            block = block->previous.get();
        }
    }
    virtual BlockIndex* getBlock(int height) const
    {
        if (height < 0 || height >= m_blocks.size())
            return nullptr;
        return m_blocks[height].get();
    }
    virtual void getLocator(std::vector<uint256>& have, BlockIndex* block) const
    {
        int step = 1;
        have.clear();
        have.reserve(32);
        if (!block)
            block = getTip();
        while (block)
        {
            have.push_back(block->getHash());
            // Stop when we have added the genesis block.
            if (block->height == 0)
                break;
            // Exponentially larger steps back, plus the genesis block.
            int height = std::max(block->height - step, 0);
            if (contains(block))
            {
                // Use O(1) CChain index if possible.
                block = getBlock(height);
            }
            else
            {
                // Otherwise, use O(log n) skiplist.
                block = block->getAncestor(height);
            }
            if (have.size() > 10)
                step *= 2;
        }
    }

private:
private:
    XUL_LOGGER_DEFINE();
    std::vector<BlockIndexPtr> m_blocks;
};


BlockChain* createBlockChain()
{
    return new BlockChainImpl;
}

BlockIndex* findLastCommonAncestor(BlockIndex* x, BlockIndex* y)
{
    if (x->height > y->height)
    {
        x = x->getAncestor(y->height);
    }
    else if (y->height > x->height)
    {
        y = y->getAncestor(x->height);
    }

    while (x != y && x && y)
    {
        x = x->previous.get();
        y = y->previous.get();
    }

    // Eventually all chain branches meet at the genesis block.
    assert(x == y);
    return x;
}

}
