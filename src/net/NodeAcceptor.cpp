
#include "NodeAcceptor.hpp"

#include "NodeInfo.hpp"
#include "PendingNode.hpp"
#include "MessageCodec.hpp"
#include "NodeManager.hpp"
#include "AppInfo.hpp"
#include "AppConfig.hpp"

#include <xul/lang/object_impl.hpp>
#include <xul/net/tcp_acceptor.hpp>
#include <xul/net/io_service.hpp>
#include <xul/log/log.hpp>
#include <map>
#include <set>
#include <assert.h>


namespace xbtc {


class NodeAcceptorImpl
    : public xul::object_impl<NodeAcceptor>
    , public xul::tcp_acceptor_listener
    , public PendingNodeHandler
{
public:
    typedef boost::intrusive_ptr<PendingNode> PendingNodePtr;
    typedef std::map<PendingNode*, PendingNodePtr> PendingNodeMap;

    explicit NodeAcceptorImpl(AppInfo* appInfo)
    {
        XUL_LOGGER_INIT("NodeAcceptor");
        XUL_DEBUG("new");
        m_appInfo = appInfo;
        m_tcpAcceptor = m_appInfo->getIOService()->create_tcp_acceptor();
        m_tcpAcceptor->set_listener(this);
    }
    virtual ~NodeAcceptorImpl()
    {
        XUL_DEBUG("delete");
        m_tcpAcceptor->set_listener(nullptr);
        m_tcpAcceptor->close();
    }
    virtual void onTick(int64_t times)
    {
        if (times % 4 == 1)
        {
            PendingNodeMap::iterator iter = m_connections.begin();
            while (iter != m_connections.end())
            {
                PendingNode* conn = iter->second.get();
                ++iter;
                conn->onTick(times);
            }
        }
    }

    virtual void removePendingNode(PendingNode* node)
    {
        removeConnection(node);
    }
    virtual void handleConnectionRequest(PendingNode* conn)
    {
        XUL_DEBUG("handleConnectionRequest " << conn);
        m_deadNodes.clear();
        return;
    }
    virtual void handleConnectionCreated(PendingNode* conn)
    {
    }

    virtual bool handleConnectionResponse(PendingNode* conn)
    {
        XUL_DEBUG("handleConnectionResponse " << conn);
        assert(false);
        return false;
    }

    virtual void on_acceptor_client( xul::tcp_acceptor* acceptor, xul::tcp_socket* newClient, const xul::inet_socket_address& sockAddr )
    {
        XUL_DEBUG("on_acceptor_client " << sockAddr);
        newClient->close();
        assert(false);
    }

    virtual void handleConnectionError(PendingNode* conn, int errcode)
    {
        XUL_DEBUG("delPendingNode " << conn);
        removeConnection(conn);
    }

    void removeConnection(PendingNode* conn)
    {
        XUL_DEBUG("removeConnection " << conn);
        m_deadNodes.clear();
        m_deadNodes[conn] = conn;
        m_connections.erase(conn);
        conn->setHandler(nullptr);
    }

    virtual int open(int port)
    {
        XUL_REL_INFO("open " << port);
        int i = 0;
        if (m_tcpAcceptor->open(port, false))
        {
            XUL_REL_INFO("open port ok " << port + i);
            return port;
        }
        XUL_REL_EVENT("failed to open port " << port);
        return -1;
    }

private:
    XUL_LOGGER_DEFINE();

    boost::intrusive_ptr<AppInfo> m_appInfo;
    boost::intrusive_ptr<xul::tcp_acceptor> m_tcpAcceptor;
    PendingNodeMap m_connections;
    PendingNodeMap m_deadNodes;
};


NodeAcceptor* createNodeAcceptor(AppInfo* appInfo)
{
    return new NodeAcceptorImpl(appInfo);
}


}
