#include "NodeInfo.hpp"

#include "AppInfo.hpp"
#include "NodeAddress.hpp"
#include "MessageCodec.hpp"

#include <xul/lang/object_impl.hpp>
#include <xul/net/inet_socket_address.hpp>
#include <xul/io/codec/message_packet_pipe.hpp>
#include <xul/net/tcp_socket.hpp>
#include <xul/net/net_interface.hpp>


namespace xbtc {


class NodeInfoImpl : public xul::object_impl<NodeInfo>
{
public:
    explicit NodeInfoImpl(xul::tcp_socket* sock, bool inboundIn, const PeerAddress& peerAddr, const xul::inet_socket_address& sockAddr)
    {
        socket = sock;
        socketAddress = sockAddr;
        inbound = inboundIn;
        connected = false;
        version = 0;
        nodeAddress.address = peerAddr;
        rtt = 0;
    }
    virtual ~NodeInfoImpl()
    {
        if (socket)
        {
            socket->destroy();
        }
    }
};


class HostNodeInfoImpl : public xul::object_impl<HostNodeInfo>
{
public:
    explicit HostNodeInfoImpl(int port)
    {
        headersSynced = false;
    }

    virtual const std::vector<xul::net_interface>& get_net_interfaces() const
    {
        return m_net_interfaces;
    }
    virtual int getPort() const
    {
        assert(m_peerAddress.port > 0);
        return m_peerAddress.port;
    }
    virtual void setPort(int port)
    {
        assert(port > 0);
        m_peerAddress.port = port;
    }
    virtual int getExternalPort() const
    {
        return m_externalPeerAddress.port;
    }
    virtual void setExternalPort(int port)
    {
        m_externalPeerAddress.port = port;
    }
    virtual const PeerAddress& getAddress() const
    {
        return m_peerAddress;
    }
    virtual const PeerAddress& getExternalAddress() const
    {
        return m_externalPeerAddress;
    }
    virtual void setExternalIP(uint32_t ip)
    {
        assert(ip > 0);
        m_peerAddress.ip = ip;
    }

    virtual int getVersionNumber() const { return m_peer_version_number; }
    virtual const std::string& getUserAgent() const { return m_userAgent; }

private:
    PeerAddress m_peerAddress;
    PeerAddress m_externalPeerAddress;
    int m_peer_version_number;
    std::string m_userAgent;
    std::vector<xul::net_interface> m_net_interfaces;
};


NodeInfo* createOutboundNodeInfo(xul::tcp_socket* sock, const PeerAddress& peerAddr)
{
    return new NodeInfoImpl(sock, false, peerAddr, toSocketAddress(peerAddr));
}

NodeInfo* createInboundNodeInfo(xul::tcp_socket* sock)
{
    xul::inet_socket_address sockAddr;
    sock->get_remote_address(sockAddr);
    return new NodeInfoImpl(sock, true, PeerAddress(sockAddr.get_ip(), sockAddr.get_port()), sockAddr);
}

HostNodeInfo* createHostNodeInfo() {
    return new HostNodeInfoImpl(8833);
}

}
