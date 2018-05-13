
#include "NodeConnector.hpp"

#include "NodeInfo.hpp"
#include "PendingNode.hpp"
#include "PeerPool.hpp"
#include "MessageCodec.hpp"
#include "NodeManager.hpp"

#include "AppInfo.hpp"
#include "AppConfig.hpp"

#include <xul/lang/object_impl.hpp>
#include <xul/io/structured_writer.hpp>
#include <xul/net/io_service.hpp>
#include <xul/net/inet_socket_address.hpp>
#include <xul/log/log.hpp>
#include <xul/macro/foreach.hpp>
#include <map>
#include <set>


namespace xbtc {


class NodeConnectorImpl : public xul::object_impl<NodeConnector>, public PendingNodeHandler
{
public:
    typedef boost::intrusive_ptr<PendingNode> PendingNodePtr;
    typedef std::map<PendingNode*, PendingNodePtr> PendingNodeMap;

    explicit NodeConnectorImpl(NodeManager* mgr)
        : m_nodeManager(mgr)
    {
        XUL_LOGGER_INIT("PeerConnector");
        XUL_DEBUG("new");
    }
    virtual ~NodeConnectorImpl()
    {
        XUL_DEBUG("delete");
        stop();
    }

    virtual void schedule()
    {        
        tryConnect();
    }

    virtual void stop()
    {
        XUL_DEBUG("stop");
        m_deadNodes.clear();
        for (const auto& node : m_nodes)
        {
            node.second->close();
        }
        m_nodes.clear();
    }

    virtual void removePendingNode(PendingNode* node)
    {
        removeConnection(node);
    }
    virtual void onTick(int64_t times)
    {
        m_deadNodes.clear();
        PendingNodeMap::iterator iter = m_nodes.begin();
        while (iter != m_nodes.end())
        {
            PendingNode* node = iter->second.get();
            ++iter;
            node->onTick(times);
        }

//        schedule();
    }
    virtual void dump(xul::structured_writer* writer, int level) const
    {

    }
    virtual void handleConnectionCreated(PendingNode* node)
    {
        XUL_DEBUG("handleConnectionCreated " << node);
        boost::intrusive_ptr<NodeInfo> nodeInfo = node->getNodeInfo();
        removeConnection(node);
        m_nodeManager->addNewNode(nodeInfo.get());
    }

    virtual void handleConnectionError(PendingNode* node, int errcode)
    {
        XUL_WARN("handleConnectionError " << node << " " << errcode);
        removeConnection(node);
    }

private:
    void removeConnection(PendingNode* node)
    {
        XUL_DEBUG("removeConnection " << node);
        m_deadNodes.clear();
        m_deadNodes[node] = node;
        m_nodes.erase(node);
        node->setHandler(nullptr);
    }
    void tryConnect()
    {
        XUL_DEBUG("tryConnect");
        PeerAddress addr;
        int32_t connectNum = m_nodeManager->getNodeShortage();
        int preconnectNum = 5;
        if (connectNum > 0)
        {
            while (preconnectNum > 0 && m_nodeManager->getAppInfo()->getNodePool()->getConnectionPeer(&addr))
            {
                doConnect(addr);
                preconnectNum--;
            }
        }
    }
    void doConnect(const PeerAddress& addr)
    {
        XUL_DEBUG("doConnect " << addr);

        if (m_nodeManager->getAppInfo()->getHostNodeInfo()->nodeAddress.address == addr ||
            m_nodeManager->getAppInfo()->getHostNodeInfo()->externalPeerAddress == addr)
        {
            XUL_DEBUG("addr self , not connect " << addr);
            return;
        }
        if (m_nodeManager->doesNodeAddressConnected(addr))
        {
            XUL_DEBUG("addr is connect, not connect again. " << addr);
            return;
        }

        NodeInfo* nodeInfo = createOutboundNodeInfo(m_nodeManager->getAppInfo()->getIOService()->create_tcp_socket(), addr);
        PendingNode* conn = createPendingNode(nodeInfo, m_nodeManager->getAppInfo());
        conn->setHandler(this);
        m_nodes[conn] = conn;
        m_nodeManager->getAppInfo()->getNodePool()->setPeerConnecting(&addr);
    }

private:
    XUL_LOGGER_DEFINE();
    boost::intrusive_ptr<NodeManager> m_nodeManager;
    PendingNodeMap m_nodes;
    PendingNodeMap m_deadNodes;
};


NodeConnector* createNodeConnector(NodeManager* mgr)
{
    return new NodeConnectorImpl(mgr);
}


}
