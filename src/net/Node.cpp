#include "Node.hpp"

#include "NodeManager.hpp"
#include "BlockSynchronizer.hpp"
#include "NodeInfo.hpp"
#include "NodeSyncInfo.hpp"
#include "MessageCodec.hpp"
#include "Messages.hpp"
#include "NodePool.hpp"
#include "flags.hpp"
#include "storage/BlockCache.hpp"
#include "storage/BlockChain.hpp"
#include "AppConfig.hpp"
#include "db.hpp"

#include <xul/lang/object_impl.hpp>
#include <xul/net/tcp_socket.hpp>
#include <xul/log/log.hpp>
#include <xul/io/data_input_stream.hpp>

#include <functional>
#include <unordered_map>

namespace xbtc {


#define XBTC_REG_MSG_HANDLER(msgtype) m_handlers[#msgtype] = std::bind(&NodeImpl::handleCommand_##msgtype, this, std::placeholders::_1, std::placeholders::_2)

std::ostream& operator<<(std::ostream& os, const Node& node)
{
    return os << node.getNodeInfo()->nodeAddress.address;
}

class NodeImpl : public xul::object_impl<Node>, public xul::tcp_socket_listener, public MessageDecoderListener
{
public:
    typedef std::function<void (const MessageHeader&, xul::data_input_stream&)> MessageHandlerFunction;
    typedef std::unordered_map<std::string, MessageHandlerFunction> MessageHandlerFunctionTable;
    explicit NodeImpl(NodeInfo* nodeInfo, NodeManager& nodeManager)
        : m_nodeInfo(nodeInfo)
        , m_nodeManager(nodeManager)
    {
        XUL_LOGGER_INIT("Node");
        XUL_DEBUG("new " << *this);
        m_nodeInfo->socket->set_listener(this);
        m_nodeInfo->messageDecoder->setListener(this);
        XBTC_REG_MSG_HANDLER(version);
        XBTC_REG_MSG_HANDLER(verack);
        XBTC_REG_MSG_HANDLER(addr);
        XBTC_REG_MSG_HANDLER(inv);
        XBTC_REG_MSG_HANDLER(getdata);
        XBTC_REG_MSG_HANDLER(notfound);
        XBTC_REG_MSG_HANDLER(getblocks);
        XBTC_REG_MSG_HANDLER(getheaders);
        XBTC_REG_MSG_HANDLER(tx);
        XBTC_REG_MSG_HANDLER(block);
        XBTC_REG_MSG_HANDLER(headers);
        XBTC_REG_MSG_HANDLER(getaddr);
        XBTC_REG_MSG_HANDLER(mempool);
        XBTC_REG_MSG_HANDLER(ping);
        XBTC_REG_MSG_HANDLER(pong);
        XBTC_REG_MSG_HANDLER(reject);
        XBTC_REG_MSG_HANDLER(filterload);
        XBTC_REG_MSG_HANDLER(filteradd);
        XBTC_REG_MSG_HANDLER(filterclear);
        XBTC_REG_MSG_HANDLER(merkleblock);
        XBTC_REG_MSG_HANDLER(alert);
        XBTC_REG_MSG_HANDLER(sendheaders);
        XBTC_REG_MSG_HANDLER(feefilter);
        XBTC_REG_MSG_HANDLER(sendcmpct);
        XBTC_REG_MSG_HANDLER(cmpctblock);
        XBTC_REG_MSG_HANDLER(getblocktxn);
        XBTC_REG_MSG_HANDLER(blocktxn);
    }
    ~NodeImpl()
    {
        XUL_DEBUG("delete " << *this);
        m_nodeInfo->messageDecoder->setListener(nullptr);
    }

    virtual void start()
    {
        XUL_DEBUG("start " << *this);
        send_getaddr();
        send_sendheaders();
        if (m_nodeInfo->nodeAddress.services & ServiceFlags::NODE_WITNESS)
        {
            send_sendcmpct(false, 2);
        }
        send_sendcmpct(false, 1);
        send_ping();
    }
    virtual void close()
    {
        m_nodeInfo->socket->destroy();
    }
    virtual NodeSyncInfo& getSyncInfo()
    {
        return m_syncInfo;
    }
    virtual void requestHeaders(std::vector<uint256>&& hashes)
    {
        XUL_EVENT("requestHeaders " << hashes.size() << " " << *this);
        GetHeadersMessage msg;
        msg.version = m_nodeManager.getAppInfo()->getHostNodeInfo()->version;
        msg.hashes = hashes;
        sendMessage(msg);
    }
    virtual void requestBlocks(const std::vector<BlockIndex*>& blocks)
    {
        XUL_EVENT("requestBlocks " << blocks.size() << " " << *this);
        assert(blocks.size() > 0);
        GetDataMessage request;
        request.inventory.reserve(blocks.size());
        for (auto block : blocks)
        {
            request.inventory.push_back(InventoryItem(MSG_BLOCK, block->getHash()));
        }
        sendMessage(request);
        m_syncInfo.recordRequestingBlocks(blocks);
    }

    virtual NodeInfo* getNodeInfo() { return m_nodeInfo.get(); }
    virtual const NodeInfo* getNodeInfo() const { return m_nodeInfo.get(); }

	virtual void on_socket_connect( xul::tcp_socket* sender )
	{
        assert(false);
	}

	virtual void on_socket_connect_failed( xul::tcp_socket* sender, int errcode )
	{
        assert(false);
	}

    void handleError(int errcode)
    {
        if (!m_nodeInfo->inbound)
        {
            if (m_nodeManager.getAppInfo()->getNodePool())
            {
                m_nodeManager.getAppInfo()->getNodePool()->setPeerDisconnected(&m_nodeInfo->getPeerAddress(), errcode, false);
            }
        }

        close();
        m_nodeManager.removeNode(this);
    }
	virtual void on_socket_receive( xul::tcp_socket* sender, unsigned char* data, size_t size ) 
	{
        XUL_DEBUG("on_socket_receive " << m_nodeInfo->socketAddress << " " << size);
        m_nodeInfo->lastDataReceiveTime.sync();
        if (!m_nodeInfo->messageDecoder->decode(data, size))
        {
            // bad message data
            handleError(-1);
            return;
        }
		receivePacket();
	}

	virtual void on_socket_receive_failed( xul::tcp_socket* sender, int errcode ) 
	{
		XUL_DEBUG("on_socket_receive_failed " << m_nodeInfo->socketAddress << " " << errcode);
		handleError(-2);
	}
    virtual void onMessageDecoded(const MessageHeader& header, const std::vector<uint8_t>& payload)
    {
        auto iter = m_handlers.find(header.command);
        if (iter == m_handlers.end())
        {
            // error
            assert(false);
            return;
        }
        m_nodeInfo->lastMessageReceiveTime.sync();
        XUL_DEBUG("onMessageDecoded msg " << header.command << " " << header.length);
        auto handler = iter->second;
        uint8_t dummybuf[1];
        xul::memory_data_input_stream is(payload.empty() ? dummybuf : payload.data(), payload.size(), false);
        handler(header, is);
    }

private:
	void receivePacket()
	{
        m_nodeInfo->socket->receive(20 * 1024);
	}
    void sendMessage(const Message& msg)
    {
        m_nodeInfo->messageSender->sendMessage(msg);
    }

    void send_getaddr()
    {
        XUL_DEBUG("send_getaddr " << *this);
        GetAddrMessage msg;
        sendMessage(msg);
    }

    void send_sendheaders()
    {
        XUL_DEBUG("send_sendheaders " << *this);
        SendHeadersMessage msg;
        sendMessage(msg);
    }

    void send_sendcmpct(bool announceUsingCMPCTBLOCK, uint64_t CMPCTBLOCKVersion)
    {
        XUL_DEBUG("send_sendcmpct " << *this);
        SendCmpctMessage msg;
        msg.announceUsingCMPCTBLOCK = announceUsingCMPCTBLOCK;
        msg.CMPCTBLOCKVersion = CMPCTBLOCKVersion;
        sendMessage(msg);
    }

    void send_ping()
    {
        XUL_DEBUG("send_ping " << *this);
        PingMessage msg;
        sendMessage(msg);
    }

    void send_pong(uint64_t nonce)
    {
        XUL_DEBUG("send_pong " << *this);
        PongMessage msg;
        msg.nonce = nonce;
        sendMessage(msg);
    }

    void send_feefilter()
    {
        XUL_DEBUG("send_feefilter " << *this);
        FeeFilterMessage msg;
        sendMessage(msg);
    }

    void handleCommand_version(const MessageHeader& header, xul::data_input_stream& is)
    {
        VersionMessage msg;
        is >> msg;
        XUL_INFO("handleCommand_version " << msg.version << " " << msg.userAgent);
    }
    void handleCommand_verack(const MessageHeader& header, xul::data_input_stream& is)
    {
        XUL_INFO("handleCommand_verack");
    }
    void handleCommand_addr(const MessageHeader& header, xul::data_input_stream& is)
    {
        AddrMessage msg;
        is >> msg;
        if (!is)
        {
            handleError(-1);
            assert(false);
            return;
        }
        XUL_DEBUG("handleCommand_addr " << msg.addresses.size());
        if (!m_nodeManager.getAppInfo()->getAppConfig()->directNode.empty())
            return;
        for (const auto& addr : msg.addresses)
        {
            m_nodeManager.getAppInfo()->getNodePool()->addPeer(&addr.address.address);
        }
    }
    void handleCommand_inv(const MessageHeader& header, xul::data_input_stream& is)
    {
        XUL_DEBUG("handleCommand_inv " << *this);
    }
    void handleCommand_getdata(const MessageHeader& header, xul::data_input_stream& is)
    {
        XUL_DEBUG("handleCommand_getdata " << *this);
    }
    void handleCommand_notfound(const MessageHeader& header, xul::data_input_stream& is)
    {
        XUL_DEBUG("handleCommand_notfound " << *this);
    }
    void handleCommand_getblocks(const MessageHeader& header, xul::data_input_stream& is)
    {
        XUL_DEBUG("handleCommand_getblocks " << *this);
    }
    void handleCommand_getheaders(const MessageHeader& header, xul::data_input_stream& is)
    {
        XUL_DEBUG("handleCommand_getheaders " << *this);
    }
    void handleCommand_tx(const MessageHeader& header, xul::data_input_stream& is)
    {
        XUL_DEBUG("handleCommand_tx " << *this);
    }
    void handleCommand_block(const MessageHeader& header, xul::data_input_stream& is)
    {
        BlockMessage msg;
        is >> msg;
        if (!is)
        {
            handleError(-1);
            assert(false);
            return;
        }
        msg.block->header.computeHash();
        m_nodeManager.getBlockSynchronizer()->handleBlock(msg.block.get(), this);
        XUL_EVENT("handleCommand_block " << msg.block->transactions.size() << " " << *this);
    }
    void handleCommand_headers(const MessageHeader& header, xul::data_input_stream& is)
    {
        HeadersMessage msg;
        is >> msg;
        if (!is)
        {
            handleError(-1);
            assert(false);
            return;
        }
        m_nodeManager.getBlockSynchronizer()->handleHeaders(msg.headers, this);
        XUL_DEBUG("handleCommand_headers " << msg.headers.size() << " " << *this);
    }
    void handleCommand_getaddr(const MessageHeader& header, xul::data_input_stream& is)
    {
        XUL_DEBUG("handleCommand_getaddr " << *this);
    }
    void handleCommand_mempool(const MessageHeader& header, xul::data_input_stream& is)
    {
        XUL_DEBUG("handleCommand_mempool " << *this);
    }
    void handleCommand_ping(const MessageHeader& header, xul::data_input_stream& is)
    {
        PingMessage msg;
        is >> msg;
        XUL_DEBUG("handleCommand_ping " << msg.nonce << " " << *this);
        send_pong(msg.nonce);
    }
    void handleCommand_pong(const MessageHeader& header, xul::data_input_stream& is)
    {
        PongMessage msg;
        is >> msg;
        XUL_DEBUG("handleCommand_pong " << msg.nonce << " " << *this);
    }
    void handleCommand_reject(const MessageHeader& header, xul::data_input_stream& is)
    {
        XUL_DEBUG("handleCommand_reject " << *this);
    }
    void handleCommand_filterload(const MessageHeader& header, xul::data_input_stream& is)
    {
        XUL_DEBUG("handleCommand_filterload " << *this);
    }
    void handleCommand_filteradd(const MessageHeader& header, xul::data_input_stream& is)
    {
        XUL_DEBUG("handleCommand_filteradd " << *this);
    }
    void handleCommand_filterclear(const MessageHeader& header, xul::data_input_stream& is)
    {
        XUL_DEBUG("handleCommand_filterclear " << *this);
    }
    void handleCommand_merkleblock(const MessageHeader& header, xul::data_input_stream& is)
    {
        XUL_DEBUG("handleCommand_merkleblock " << *this);
    }
    void handleCommand_alert(const MessageHeader& header, xul::data_input_stream& is)
    {
        XUL_DEBUG("handleCommand_alert " << *this);
    }
    void handleCommand_sendheaders(const MessageHeader& header, xul::data_input_stream& is)
    {
        XUL_DEBUG("handleCommand_sendheaders " << *this);
    }
    void handleCommand_feefilter(const MessageHeader& header, xul::data_input_stream& is)
    {
        FeeFilterMessage msg;
        is >> msg;
        XUL_DEBUG("handleCommand_feefilter " << msg.feeRate << " " << *this);
    }
    void handleCommand_sendcmpct(const MessageHeader& header, xul::data_input_stream& is)
    {
        SendCmpctMessage msg;
        is >> msg;
        XUL_DEBUG("handleCommand_sendcmpct " << xul::make_tuple((int)msg.announceUsingCMPCTBLOCK, msg.CMPCTBLOCKVersion));
    }
    void handleCommand_cmpctblock(const MessageHeader& header, xul::data_input_stream& is)
    {
        XUL_DEBUG("handleCommand_cmpctblock " << *this);
    }
    void handleCommand_getblocktxn(const MessageHeader& header, xul::data_input_stream& is)
    {
        XUL_DEBUG("handleCommand_cmpctblock " << *this);
    }
    void handleCommand_blocktxn(const MessageHeader& header, xul::data_input_stream& is)
    {
        XUL_DEBUG("handleCommand_cmpctblock " << *this);
    }

private:
    XUL_LOGGER_DEFINE();
    boost::intrusive_ptr<NodeInfo> m_nodeInfo;
    NodeManager& m_nodeManager;
    MessageHandlerFunctionTable m_handlers;
    NodeSyncInfo m_syncInfo;
};


Node* createNode(NodeInfo* nodeInfo, NodeManager& nodeManager)
{
    return new NodeImpl(nodeInfo, nodeManager);
}


}
