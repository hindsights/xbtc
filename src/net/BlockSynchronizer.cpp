
#include "BlockSynchronizer.hpp"
#include "Node.hpp"
#include "NodeManager.cpp"
#include "NodeSyncInfo.hpp"
#include "NodeInfo.hpp"
#include "AppInfo.hpp"
#include "storage/BlockCache.hpp"
#include "storage/BlockChain.hpp"
#include "data/Block.hpp"
#include "AppConfig.hpp"
#include "db.hpp"

#include <xul/lang/object_impl.hpp>
#include <xul/net/tcp_socket.hpp>
#include <xul/net/inet_socket_address.hpp>
#include <xul/log/log.hpp>
#include <xul/util/time_counter.hpp>
#include <xul/data/big_number_io.hpp>
#include <map>


namespace xbtc {


class BlockSynchronizerImpl : public xul::object_impl<BlockSynchronizer>
{
public:
    typedef boost::intrusive_ptr<Node> NodePtr;
    typedef std::map<Node*, NodePtr> NodeMap;

    explicit BlockSynchronizerImpl(NodeManager& nodeManager)
        : m_nodeManager(nodeManager), m_requestingHeaders(false)
    {
        XUL_LOGGER_INIT("BlockSynchronizer");
        XUL_DEBUG("new");
    }
    virtual ~BlockSynchronizerImpl()
    {
        XUL_DEBUG("delete");
    }

    virtual void onTick(int64_t times)
    {
        checkHeaderRequestTimeout();
        chooseHeadersRueqster(false);
        scheduleRequestHeaders();
    }

    void checkHeaderRequestTimeout()
    {
        if (m_requestingHeaders && m_lastHeadersRequestTime.elapsed() > 5000)
        {
            if (m_headersRueqster && m_headersRueqster->getNodeInfo()->lastDataReceiveTime.elapsed() > 3000)
            {
                XUL_EVENT("onTick idle header requester " << *m_headersRueqster);
                m_requestingHeaders = false;
                m_nodeManager.removeNode(m_headersRueqster.get());
            }
        }
    }

    void scheduleRequestHeaders()
    {
        if (m_requestingHeaders)
            return;
        if (m_lastHeadersRequestTime.elapsed() < 10000)
            return;
        requestHeaders(nullptr);
    }

    virtual void addNode(Node* node)
    {
        XUL_DEBUG("addNode " << m_nodes.size() << " " << *node);
        m_nodes[node] = node;
        chooseHeadersRueqster(false);
    }
    virtual void removeNode(Node* node)
    {
        XUL_DEBUG("removeNode " << m_nodes.size() << " " << *node);
        m_nodes.erase(node);
        if (node == m_headersRueqster.get())
        {
            chooseHeadersRueqster(true);
        }
    }
    virtual void handleHeaders(std::vector<BlockHeader>& headers, Node* node)
    {
        m_requestingHeaders = false;
        NodeSyncInfo& syncInfo = node->getSyncInfo();
        BlockIndex* block;
        for (auto& header : headers)
        {
            header.computeHash();
            block = m_nodeManager.getAppInfo()->getBlockCache()->addBlockIndex(header);
        }
        if (!headers.empty())
        {
            m_lastHeadersReceiveTime.sync();
            assert(block);
            syncInfo.updateBlockAvailability(headers[headers.size() - 1].hash, block, m_nodeManager.getAppInfo()->getBlockCache());
            XUL_EVENT("handleHeaders request again " << xul::make_tuple(headers.size(), block->height) << " " << *node);
            requestHeaders(block);
        }
        if (headers.size() < 2000 && !syncInfo.lastDownloadBlock)
        {
            XUL_EVENT("handleCommand_headers start request block " << headers.size() << " " << *node);
            checkRequestBlocks(node);
        }
    }
    virtual void handleBlock(Block* block, Node* node)
    {
        XUL_EVENT("handleBlock " << block->header.merkleRootHash << " " << *node);
        for (auto& tx : block->transactions)
        {
            tx.computeHash();
        }
        node->getSyncInfo().removeBlockRecord(block);
        BlockIndex* blockIndex = m_nodeManager.getAppInfo()->getBlockCache()->addBlock(block);
        XUL_EVENT("handleBlock index " << blockIndex->height << " " << *node);
        if (blockIndex->height == 33275)
        {
            XUL_EVENT("handleBlock check point " << blockIndex->height << " " << *node);
        }
        checkRequestBlocks(node);
    }

private:
    void checkRequestBlocks(Node* node)
    {
        if (!node->getSyncInfo().isRequestingBlock())
        {
            requestBlocks(node);
        }
    }
    void requestBlocks(Node* node)
    {
        std::vector<BlockIndex*> blocks;
        node->getSyncInfo().findBlocksToDownload(blocks, 128, m_nodeManager.getAppInfo()->getBlockCache(), m_nodeManager.getAppInfo()->getAppConfig());
        if (blocks.size() > 0)
        {
            node->requestBlocks(blocks);
        }
    }
    void chooseHeadersRueqster(bool forced)
    {
        if (m_headersRueqster && !forced)
            return;
        m_headersRueqster = findHeadersRueqster();
        requestHeaders(nullptr);
    }
    void requestHeaders(BlockIndex* block)
    {
        if (!m_headersRueqster)
            return;
        XUL_DEBUG("requestHeaders " << m_nodes.size() << " " << *m_headersRueqster);
        if (block == nullptr)
        {
            block = m_nodeManager.getAppInfo()->getBlockCache()->getBestHeader();
            assert(block);
            block = block->previous.get();
        }
        std::vector<uint256> hashes;
        m_nodeManager.getAppInfo()->getBlockCache()->getChain()->getLocator(hashes, block);
        m_headersRueqster->requestHeaders(std::move(hashes));
        m_requestingHeaders = true;
        m_lastHeadersRequestTime.sync();
    }
    Node* findHeadersRueqster()
    {
        XUL_DEBUG("findHeadersRueqster " << m_nodes.size());
        if (m_nodes.empty())
            return nullptr;
        // wait 2 seconds or there is enough nodes to collect as much as fast nodes
        if (m_startTime.elapsed() < 2000 && m_nodes.size() < 5)
            return nullptr;
        std::multimap<int, Node*> nodes;
        for (const auto& item : m_nodes)
        {
            Node* node = item.second.get();
            if (!node->getNodeInfo()->inbound)
            {
                nodes.insert(std::make_pair(node->getNodeInfo()->rtt, node));
            }
        }
        if (!nodes.empty())
        {
            Node* node = nodes.begin()->second;
            XUL_EVENT("findHeadersRueqster " << node->getNodeInfo()->rtt << " " << *node);
            return node;
        }
        return m_nodes.begin()->second.get();
    }

private:
    XUL_LOGGER_DEFINE();
    NodeManager& m_nodeManager;
    NodeMap m_nodes;
    NodePtr m_headersRueqster;
    bool m_requestingHeaders;
    xul::time_counter m_startTime;
    xul::time_counter m_lastHeadersRequestTime;
    xul::time_counter m_lastHeadersReceiveTime;
};


void NodeSyncInfo::processBlockAvailability(BlockCache* cache)
{
    if (lastUnknownBlockHash.is_null())
        return;
    BlockIndex* block = cache->getBlockIndex(lastUnknownBlockHash);
    if (block && block->chainWork > 0)
    {
        if (!bestKnownBlock || block->chainWork >= bestKnownBlock->chainWork)
            bestKnownBlock = block;
        lastUnknownBlockHash.clear();
    }
}
void NodeSyncInfo::updateBlockAvailability(const uint256& hash, BlockIndex* block, BlockCache* cache)
{
    processBlockAvailability(cache);
    if (block && block->chainWork > 0)
    {
        if (!bestKnownBlock || block->chainWork >= bestKnownBlock->chainWork)
            bestKnownBlock = block;
    }
    else
    {
        lastUnknownBlockHash = hash;
    }
}
void NodeSyncInfo::findBlocksToDownload(std::vector<BlockIndex*>& blocks, int count, BlockCache* cache, const AppConfig* config)
{
    processBlockAvailability(cache);
    BlockChain* chain = cache->getChain();
    if (!bestKnownBlock || bestKnownBlock->chainWork < chain->getTip()->chainWork
        || bestKnownBlock->chainWork < config->minimumChainWork)
    {
        // This peer has nothing interesting.
        return;
    }
    if (!lastCommonBlock)
    {
        // Bootstrap quickly by guessing a parent of our best tip is the forking point.
        // Guessing wrong in either direction is not a problem.
        lastCommonBlock = chain->getBlock(std::min(bestKnownBlock->height, chain->getHeight()));
    }
    lastCommonBlock = findLastCommonAncestor(lastCommonBlock.get(), bestKnownBlock.get());
    if (lastCommonBlock == bestKnownBlock)
        return;
    int windowEnd = lastCommonBlock->height + BLOCK_DOWNLOAD_WINDOW;
    int maxHeight = std::min<int>(bestKnownBlock->height, windowEnd + 1);
    BlockIndex* block = lastCommonBlock.get();
    int realCount = std::min(maxHeight - block->height, std::max<int>(count, 128));
    blocks.resize(realCount);
    block = bestKnownBlock->getAncestor(block->height + realCount);
    assert(block);
    blocks[realCount - 1] = block;
    lastDownloadBlock = block;
    lastCommonBlock = block;
    for (int i = realCount - 1; i > 0; --i)
    {
        blocks[i - 1] = blocks[i]->previous.get();
    }
}

void NodeSyncInfo::recordRequestingBlocks(const std::vector<BlockIndex*>& blocks)
{
    assert(requestingBlocks.empty());
    for (auto block : blocks)
    {
        assert(!block->getHash().is_null());
        requestingBlocks[block->getHash()] = block;
    }
}
void NodeSyncInfo::removeBlockRecord(const xbtc::Block* block)
{
    assert(!block->getHash().is_null());
    int count = requestingBlocks.erase(block->getHash());
    assert(count == 1);
}

BlockSynchronizer* createBlockSynchronizer(NodeManager& nodeManager)
{
    return new BlockSynchronizerImpl(nodeManager);
}


}
