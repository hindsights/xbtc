#include "NodeManager.hpp"
#include "PeerPool.hpp"
#include "NodeInfo.hpp"
#include "Node.hpp"
#include "AppInfo.hpp"
#include "NodeConnector.hpp"
#include "AppConfig.hpp"
#include "BlockSynchronizer.hpp"
#include "storage/ChainParams.hpp"

#include <xul/lang/object_impl.hpp>
#include <xul/lang/object_base.hpp>
#include <xul/log/log.hpp>
#include <xul/macro/foreach.hpp>
#include <xul/net/inet_socket_address.hpp>
#include <xul/net/name_resolver.hpp>
#include <xul/net/io_service.hpp>

#include <map>
#include <set>
#include <algorithm>
#include <vector>


namespace xbtc {



class PeerDiscoverer;
class PeerDiscovererListener
{
public:
    virtual ~PeerDiscovererListener() {}
    virtual void onPeerDiscovered(PeerDiscoverer* sender, const std::vector<xul::inet4_address>& addrs) = 0;
};

class PeerDiscoverer : public xul::object_base, public xul::name_resolver_listener
{
public:
    explicit PeerDiscoverer(AppInfo* appInfo) : m_appInfo(appInfo), m_listener(nullptr), m_times(0)
    {
        XUL_LOGGER_INIT("NodeManager");
        XUL_DEBUG("new");
        m_nameResolver = m_appInfo->getIOService()->create_name_resolver();
        m_nameResolver->set_listener(this);
    }
    virtual ~PeerDiscoverer()
    {
        XUL_DEBUG("delete");
    }

    void setListener(PeerDiscovererListener* listener)
    {
        m_listener = listener;
    }

    void onTick(int64_t times)
    {
    }

    bool start()
    {
        request();
        return true;
    }
    void stop()
    {
        m_nameResolver->destroy();
    }
    virtual void on_resolver_address(xul::name_resolver* sender, const std::string& name, int errcode, const std::vector<xul::inet4_address>& addrs)
    {
        XUL_APP_REL_EVENT("on_resolver_address " << name << " " << addrs.size());
        if (errcode == 0 && !addrs.empty() && m_listener != nullptr)
        {
            m_listener->onPeerDiscovered(this, addrs);
        }
        request();
    }
private:
    void request()
    {
        const auto& seeds = m_appInfo->getChainParams()->dnsSeeds;
        if (m_times >= seeds.size())
            return;
        m_nameResolver->async_resolve(seeds[m_times]);
        ++m_times;
    }
private:
    XUL_LOGGER_DEFINE();
    boost::intrusive_ptr<AppInfo> m_appInfo;
    boost::intrusive_ptr<xul::name_resolver> m_nameResolver;
    PeerDiscovererListener* m_listener;
    int m_times;
};


class NodeManagerImpl : public xul::object_impl<NodeManager>, public PeerDiscovererListener
{
public:
    typedef boost::intrusive_ptr<Node> NodePtr;
    typedef std::map<Node*, NodePtr> NodeMap;
    typedef std::map<PeerAddress, NodePtr> AddressIndexedNodeMap;

    explicit NodeManagerImpl(AppInfo* appInfo) : m_appInfo(appInfo)
    {
        XUL_LOGGER_INIT("NodeManager");
        XUL_DEBUG("new");
        m_peerDiscoverer = new PeerDiscoverer(m_appInfo.get());
        m_peerDiscoverer->setListener(this);
        m_nodeConnector = createNodeConnector(this);
        m_blockSynchronizer = createBlockSynchronizer(*this);
    }
    virtual ~NodeManagerImpl()
    {
        XUL_DEBUG("delete");
        stop();
    }

    virtual void start()
    {
        PeerAddress addr;
        if (!m_appInfo->getAppConfig()->directNode.empty() && addr.parse(m_appInfo->getAppConfig()->directNode))
        {
            m_appInfo->getNodePool()->addPeer(&addr);
        }
        else
        {
            m_peerDiscoverer->start();
        }
        m_nodeConnector->schedule();
    }
    virtual void stop()
    {
        m_peerDiscoverer->stop();
    }
    virtual void schedule()
    {
        m_nodeConnector->schedule();
    }
    virtual void onTick(int64_t times)
    {
        m_deadNodes.clear();
        m_nodeConnector->onTick(times);
        m_peerDiscoverer->onTick(times);
        m_blockSynchronizer->onTick(times);
    }
    virtual int getNodeShortage() const
    {
        return m_appInfo->getAppConfig()->maxNodeCount - static_cast<int>(m_nodes.size());
    }
    virtual AppInfo* getAppInfo() { return m_appInfo.get(); }

    virtual Node* addNewNode(NodeInfo* nodeInfo)
    {
        XUL_INFO("addNewNode" << nodeInfo->nodeAddress.address);
        Node* node = createNode(nodeInfo, *this);
        assert(m_nodes.find(node) == m_nodes.end());
        m_nodes[node] = node;
        node->start();
        m_blockSynchronizer->addNode(node);
        if (!nodeInfo->inbound)
        {
            assert(m_addressIndexedNodes.find(nodeInfo->nodeAddress.address) == m_addressIndexedNodes.end());
            m_addressIndexedNodes[nodeInfo->nodeAddress.address] = node;
        }
        return node;
    }
    virtual void removeNode(Node* node)
    {
        m_deadNodes.clear();
        m_deadNodes[node] = node;
        m_blockSynchronizer->removeNode(node);
        node->close();
        if (!node->getNodeInfo()->inbound)
        {
            assert(m_addressIndexedNodes.find(node->getNodeInfo()->nodeAddress.address) != m_addressIndexedNodes.end());
            m_addressIndexedNodes.erase(node->getNodeInfo()->nodeAddress.address);
        }
        m_nodes.erase(node);
    }
    virtual BlockSynchronizer* getBlockSynchronizer()
    {
        return m_blockSynchronizer.get();
    }

    virtual void onPeerDiscovered(PeerDiscoverer* sender, const std::vector<xul::inet4_address>& addrs)
    {
        NodePool* pool = m_appInfo->getNodePool();
        for (const auto& addr: addrs) {
            PeerAddress peerAddr(addr.get_address(), m_appInfo->getChainParams()->defaultPort);
            pool->addPeer(&peerAddr);
        }
        m_nodeConnector->schedule();
    }

    virtual bool doesNodeAddressConnected(const PeerAddress& addr)
    {
        return m_addressIndexedNodes.find(addr) != m_addressIndexedNodes.end();
    }
private:
    XUL_LOGGER_DEFINE();
    boost::intrusive_ptr<AppInfo> m_appInfo;
    boost::intrusive_ptr<PeerDiscoverer> m_peerDiscoverer;
    boost::intrusive_ptr<NodeConnector> m_nodeConnector;
    NodeMap m_nodes;
    NodeMap m_deadNodes;
    AddressIndexedNodeMap m_addressIndexedNodes;
    boost::intrusive_ptr<BlockSynchronizer> m_blockSynchronizer;
};


NodeManager* createNodeManager(AppInfo* appInfo)
{
    return new NodeManagerImpl(appInfo);
}


}
