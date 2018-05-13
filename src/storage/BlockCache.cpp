#include "BlockCache.hpp"
#include "BlockStorage.hpp"
#include "BlockChain.hpp"
#include "Validator.hpp"
#include "CoinView.hpp"
#include "data/Block.hpp"
#include "AppConfig.hpp"
#include "ChainParams.hpp"
#include "Compatibility.hpp"
#include "Consensus.hpp"

#include <xul/lang/object_impl.hpp>
#include <xul/data/big_number_io.hpp>
#include <xul/data/date_time.hpp>
#include <xul/util/time_counter.hpp>
#include <xul/log/log.hpp>
#include <deque>
#include <unordered_map>
#include <map>


namespace xbtc {


class BlockCacheImpl : public xul::object_impl<BlockCache>
{
public:
    typedef std::vector<BlockIndexPtr> BlockIndexList;
    typedef std::multimap<uint256, BlockIndexPtr> UnlinkedBlockIndexChain;

    explicit BlockCacheImpl(const AppConfig* config, BlockStorage* storage, const ChainParams* chainParams)
        : m_config(config), m_storage(storage), m_chainParams(chainParams)
    {
        XUL_LOGGER_INIT("BlockCache");
        XUL_REL_EVENT("new");
        m_chain = createBlockChain();
        m_blocks = std::make_shared<BlockIndexMap>();
        m_dirtyBlocks = std::make_shared<BlockIndexMap>();
        m_coinView = createCoinView(config);
        m_validator = createValidator(m_coinView.get(), config);
    }
    ~BlockCacheImpl()
    {
        XUL_REL_EVENT("delete");
    }

    virtual bool load()
    {
        XUL_DEBUG("load cache:");
        BlockIndexesData data;
        if (!m_storage->load(data))
            return false;
        m_blocks = std::move(data.blocks);
        sortOutBlocks();
        loadGenesisBlock();
        m_coinView->load();
        loadChainTip();
        return true;
    }
    virtual BlockIndex* addBlock(Block* block)
    {
        BlockIndex* blockIndex = addBlockIndex(block->header);
        updateBlockIndex(blockIndex, block);
        saveBlock(block, blockIndex);
        if (blockIndex->height > 0)
        {
            if (!m_validator->validateBlock(block, blockIndex))
                return nullptr;
            m_chain->setTip(blockIndex);
            if (blockIndex->height == 91812)
            {
                m_block91812 = block;
            }
            else if (blockIndex->height == 91842)
            {
                m_block91842 = block;
            }
            m_coinView->setBestBlockHash(blockIndex->getHash(), blockIndex->height);
            updateCoins(block, blockIndex);
        }
        return blockIndex;
    }
    virtual BlockIndex* addBlockIndex(const BlockHeader& header)
    {
        assert(!header.hash.is_null());
        BlockIndexPtr& block = (*m_blocks)[header.hash];
        if (block)
            return block.get();
        if (!m_validator->validateBlockHeader(header))
            return nullptr;
        block = createBlockIndex(header);
        if (!block->header.previousBlockHash.is_null())
        {
            auto iter = m_blocks->find(block->header.previousBlockHash);
            if (iter != m_blocks->end())
            {
                block->previous = iter->second;
                block->height = block->previous->height + 1;
                if (block->height % 20000 == 1)
                {
                    XUL_EVENT("addBlockIndex " << block->height);
                }
                if (block->height == 91812)
                {
                    m_index91812 = block;
                }
                else if (block->height == 91842)
                {
                    m_index91842 = block;
                }
            }
        }
        block->raiseValidity(BlockStatus::BLOCK_VALID_TREE);
        updateBlockInfo(block.get());
        markDirtyBlock(block.get());
        return block.get();
    }
    virtual BlockIndex* getBlockIndex(const uint256& hash)
    {
        assert(index >= 0);
        auto iter = m_blocks->find(hash);
        if (iter == m_blocks->end())
            return nullptr;
        return iter->second.get();
    }
    virtual const ChainParams* getChainParams() const
    {
        return m_chainParams.get();
    }
    virtual BlockIndex* getTip()
    {
        return m_chain->getTip();
    }
    virtual BlockChain* getChain()
    {
        return m_chain.get();
    }
    virtual CoinView* getCoinView()
    {
        return m_coinView.get();
    }
    virtual BlockIndex* getBestHeader()
    {
        return m_bestHeader.get();
    }
    virtual void getLocator(std::vector<uint256>& have, const BlockIndex* block) const
    {
    }
private:
    bool loadChainTip()
    {
        const uint256& bestBlockHash = m_coinView->getBestBlockHash();
        if (bestBlockHash.is_null())
        {
            if (m_blocks->size() == 1)
                return true;
            XUL_EVENT("loadChainTip null best block hash " << m_blocks->size());
            return false;
        }
        auto iter = m_blocks->find(bestBlockHash);
        if (iter == m_blocks->end())
        {
            XUL_EVENT("loadChainTip invalid best block hash " << bestBlockHash << " " << m_blocks->size());
            assert(false);
            return false;
        }
        m_chain->setTip(iter->second.get());
        XUL_EVENT("loadChainTip set best block hash " << xul::make_tuple(m_blocks->size(), m_chain->getHeight(), iter->second->height) << " " << bestBlockHash);
        return true;
    }
    bool updateCoins(const Block* block, const BlockIndex* blockIndex)
    {
        if (!m_validator->verifyTransactions(block, blockIndex))
            return false;
        for (auto tx : block->transactions)
        {
            updateCoins(&tx, blockIndex);
        }
        return true;
    }
    void updateCoins(const Transaction* tx, const BlockIndex* blockIndex)
    {
        m_coinView->transfer(tx, blockIndex->height);
    }
    void loadGenesisBlock()
    {
        auto iter = m_blocks->find(m_chainParams->genesisBlock->getHash());
        if (iter != m_blocks->end())
        {
            // m_chainParams->genesisBlockIndex = iter->second;
            m_chain->setTip(iter->second.get());
            return;
        }
        BlockIndex* block = addBlockIndex(m_chainParams->genesisBlock->header);
        updateBlockIndex(block, m_chainParams->genesisBlock.get());
        saveBlock(m_chainParams->genesisBlock.get(), block);
        // m_chainParams->genesisBlockIndex = block;
        m_chain->setTip(block);
        flush();
    }
    void updateBlockIndex(BlockIndex* blockIndex, const Block* block)
    {
        assert(blockIndex->transactionCount == 0 || blockIndex->transactionCount == block->transactions.size());
        blockIndex->transactionCount = block->transactions.size();
    }
    void saveBlock(Block* block, BlockIndex* blockIndex)
    {
        DiskBlockPos pos = m_storage->writeBlock(block, blockIndex);
        blockIndex->setDiskPosition(pos);
        blockIndex->status |= BLOCK_HAVE_DATA;
        // check witness
        blockIndex->raiseValidity(BlockStatus::BLOCK_VALID_TRANSACTIONS);
        markDirtyBlock(blockIndex);
    }
    void markDirtyBlock(BlockIndex* block)
    {
        (*m_dirtyBlocks)[block->getHash()] = block;
        checkFlush();
    }
    void checkFlush()
    {
        if (m_dirtyBlocks->size() >= 100000 || m_lastFlushTime.elapsed() > 5000)
        {
            flush();
        }
    }
    void flush()
    {
        m_lastFlushTime.sync();
        auto data = std::make_shared<BlockIndexesData>();
        data->blocks = std::move(m_dirtyBlocks);
        m_dirtyBlocks = std::make_shared<BlockIndexMap>();
        m_storage->flush(data);
    }
    void updatePreviousBlock()
    {
        xul::time_counter starttime;
        for (const auto& item : *m_blocks)
        {
            const uint256& hash = item.first;
            BlockIndex* block = item.second.get();
            assert(block->getHash() == hash);
            assert(!block->previous);
            auto iter = m_blocks->find(block->header.previousBlockHash);
            if (iter != m_blocks->end())
            {
                block->previous = iter->second;
            }
        }
        XUL_DEBUG("updatePreviousBlock " << xul::make_tuple(m_blocks->size(), starttime.elapsed()));
    }
    void updateBlockInfo(BlockIndex* block)
    {
        block->chainWork = Consensus::calcBlockProof(block->header.bits);
        block->maxTime = block->header.timestamp;
        block->chainTransactionCount = block->transactionCount;
        if (block->previous)
        {
            block->chainTransactionCount += block->previous->chainTransactionCount;
            block->chainWork += block->previous->chainWork;
            if (block->maxTime < block->previous->maxTime)
                block->maxTime = block->previous->maxTime;
        }
        if (!m_bestHeader || (m_bestHeader->chainWork < block->chainWork))
        {
            m_bestHeader = block;
        }
    }
    void updateChainWork()
    {
        xul::time_counter starttime;
        std::vector<std::pair<int, BlockIndex*> > heightSortedBlocks;
        heightSortedBlocks.reserve(m_blocks->size());
        for (const auto& item : *m_blocks)
        {
            BlockIndex* block = item.second.get();
            heightSortedBlocks.push_back(std::make_pair(block->height, block));
        }
        XUL_EVENT("updateChainWork 1 " << xul::make_tuple(m_blocks->size(), starttime.elapsed()));
        std::sort(heightSortedBlocks.begin(), heightSortedBlocks.end());
        XUL_EVENT("updateChainWork 2 " << xul::make_tuple(m_blocks->size(), starttime.elapsed()));
        for (const auto& item : heightSortedBlocks)
        {
            BlockIndex* block = item.second;
            assert(block->height == item.first);
            updateBlockInfo(block);
        }
        XUL_EVENT("updateChainWork " << xul::make_tuple(m_blocks->size(), starttime.elapsed())
            << " " << m_bestHeader << " " << (m_bestHeader ? m_bestHeader->chainWork : uint256()));
        assert(m_bestHeader || m_blocks->empty());
        if (m_bestHeader)
        {
            XUL_EVENT("updateChainWork best header " << m_bestHeader->chainWork << " " << m_bestHeader->height
                << " " << m_bestHeader->chainTransactionCount << " " << xul::unix_time::from_utc_time(m_bestHeader->maxTime));
        }
    }
    void validateBlockIndexes()
    {
        auto iter = m_blocks->begin();
        while (iter != m_blocks->end())
        {
            BlockIndex* block = iter->second.get();
            if (!m_validator->validateBlockIndex(block))
            {
                XUL_WARN("validateBlockIndexes invalid block " << block->getHash() << " " << block->height);
                m_blocks->erase(iter++);
            }
            else
            {
                ++iter;
            }
        }
    }
    void sortOutBlocks()
    {
        XUL_DEBUG("sortOutBlocks " << xul::make_tuple(m_blocks->size(), 0));
        validateBlockIndexes();
        updatePreviousBlock();
        updateChainWork();
    }
private:
    XUL_LOGGER_DEFINE();
    BlockIndexList m_indexes;
    BlockIndexMapPtr m_blocks;
    BlockIndexPtr m_bestHeader;
    boost::intrusive_ptr<const AppConfig> m_config;
    boost::intrusive_ptr<BlockStorage> m_storage;
    boost::intrusive_ptr<Validator> m_validator;
    boost::intrusive_ptr<BlockChain> m_chain;
    boost::intrusive_ptr<const ChainParams> m_chainParams;
    BlockIndexMapPtr m_dirtyBlocks;
    xul::time_counter m_lastFlushTime;
    boost::intrusive_ptr<CoinView> m_coinView;
    BlockIndexPtr m_index91812;
    BlockIndexPtr m_index91842;
    BlockPtr m_block91812;
    BlockPtr m_block91842;
};


BlockCache* createBlockCache(const AppConfig* config, BlockStorage* storage, const ChainParams* chainParams)
{
    return new BlockCacheImpl(config, storage, chainParams);
}


}
