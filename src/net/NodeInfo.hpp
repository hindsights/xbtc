#pragma once

#include "NodeAddress.hpp"
#include "MessageSender.hpp"
#include "MessageCodec.hpp"
#include <xul/net/inet_socket_address.hpp>
#include <xul/net/tcp_socket.hpp>
#include <xul/lang/object.hpp>
#include <xul/lang/object_ptr.hpp>
#include <xul/util/time_counter.hpp>
#include <vector>
#include <string>


namespace xbtc {


class NodeInfo : public xul::object
{
public:
    boost::intrusive_ptr<xul::tcp_socket> socket;
    boost::intrusive_ptr<MessageSender> messageSender;
    boost::intrusive_ptr<MessageDecoder> messageDecoder;
    xul::inet_socket_address socketAddress;
    NodeAddress nodeAddress;
    std::string userAgent;
    xul::time_counter startTime;
    xul::time_counter lastDataReceiveTime;
    xul::time_counter lastMessageReceiveTime;
    bool inbound;
    bool connected;
    int version;
    int rtt;

    const PeerAddress& getPeerAddress() const { return nodeAddress.address; }
};

class HostNodeInfo : public xul::object
{
public:
    std::vector<xul::inet4_address> ipAddresses;
    NodeAddress nodeAddress;
    PeerAddress externalPeerAddress;
    std::string userAgent;
    int version;
    bool headersSynced;
};

NodeInfo* createOutboundNodeInfo(xul::tcp_socket* sock, const PeerAddress& addr);
NodeInfo* createInboundNodeInfo(xul::tcp_socket* sock);
HostNodeInfo* createHostNodeInfo();


}
