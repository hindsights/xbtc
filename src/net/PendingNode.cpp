
#include "PendingNode.hpp"

#include "NodeInfo.hpp"
#include "PendingNode.hpp"
#include "MessageSender.hpp"
#include "MessageCodec.hpp"
#include "AppInfo.hpp"
#include "AppConfig.hpp"
#include "PeerPool.hpp"
#include "Messages.hpp"
#include "storage/ChainParams.hpp"
#include "version.hpp"
#include "flags.hpp"

#include <xul/lang/object_impl.hpp>
#include <xul/net/tcp_socket.hpp>
#include <xul/net/inet_socket_address.hpp>
#include <xul/io/codec/message_packet_pipe.hpp>
#include <xul/text/hex_encoding.hpp>
#include <xul/util/time_counter.hpp>
#include <xul/io/data_input_stream.hpp>
#include <xul/log/log.hpp>
#include <xul/util/time_counter.hpp>
#include <xul/util/random.hpp>


namespace xbtc {


class PendingNodeImpl : public xul::object_impl<PendingNode>, public xul::tcp_socket_listener, public MessageDecoderListener
{
public:
	explicit PendingNodeImpl(NodeInfo* nodeInfo, AppInfo* appInfo)
		: m_nodeInfo(nodeInfo)
		, m_appInfo(appInfo)
		, m_handler(nullptr)
		, m_closed(false)
	{
		XUL_LOGGER_INIT("PendingNode");
		XUL_DEBUG("new");
		m_nodeInfo->messageSender = createMessageSender(m_nodeInfo.get(), m_appInfo->getMessageEncoder());
		m_nodeInfo->messageDecoder = createMessageDecoder(appInfo->getChainParams()->protocolMagic);
        m_nodeInfo->messageDecoder->setListener(this);
		m_nodeInfo->socket->set_listener(this);
		if (!m_nodeInfo->inbound)
		{
			m_nodeInfo->socket->connect(m_nodeInfo->socketAddress);
		}
		else
		{
			receivePacket();
		}
	}
	virtual ~PendingNodeImpl()
	{
		XUL_DEBUG("delete");
		if (!m_nodeInfo->inbound && !m_nodeInfo->connected)
		{
			if (m_appInfo->getNodePool())
			{
				m_appInfo->getNodePool()->setPeerDisconnected(&m_nodeInfo->nodeAddress.address, 0, false);
			}
		}
		checkClose();
	}

	virtual void close()
	{
		XUL_DEBUG("close");
		//assert(!m_nodeInfo->connected);
		checkClose();
		m_closed = true;
	}
	void checkClose()
	{
		if (!m_nodeInfo->connected)
		{
			m_nodeInfo->socket->destroy();
		}
	}

	virtual void onTick(int64_t times)
	{
		if (m_startTime.get_elapsed32() > 10000 && !m_closed)
		{
			XUL_DEBUG("handshake timeout " << m_startTime.get_elapsed32());
			handleError(-1);
		}
	}
	virtual NodeInfo* getNodeInfo()
	{
		return m_nodeInfo.get();
    }
    
    virtual void setHandler(PendingNodeHandler* handler)
    {
        m_handler = handler;
    }

    virtual void onMessageDecoded(const MessageHeader& header, const std::vector<uint8_t>& payload)
    {
        assert(header.length == payload.size());
        XUL_DEBUG("onMessageDecoded " << header.command << " " << payload.size());
        uint8_t dummybuf[1];
        xul::memory_data_input_stream is(payload.empty() ? dummybuf : payload.data(), payload.size(), false);
        if (header.command == "version")
        {
            handleCommand_version(header, is);
			return;
        }
        if (header.command == "verack")
        {
            handleCommand_verack(header, is);
			return;
        }
        if (header.command == "reject")
        {
            handleCommand_reject(header, is);
            return;
        }
        XUL_REL_WARN("onMessageDecoded error " << header.command << " " << header.length);
        assert(false);
    }
	virtual void on_socket_connect( xul::tcp_socket* sender )
	{
		XUL_DEBUG("on_socket_connect " << m_nodeInfo->socketAddress);
		assert(!m_nodeInfo->inbound);
        m_nodeInfo->rtt = m_nodeInfo->startTime.elapsed();
		m_nodeInfo->socket->receive(20 * 1024);
		XUL_DEBUG("send handshake request");
		sendVersion();
	}

	virtual void on_socket_connect_failed( xul::tcp_socket* sender, int errcode )
	{
		XUL_DEBUG("on_socket_connect_failed " << m_nodeInfo->socketAddress << " " << errcode);
		assert(!m_nodeInfo->inbound);
		handleError(-1);
	}

	virtual void on_socket_receive( xul::tcp_socket* sender, unsigned char* data, size_t size ) 
	{
		XUL_DEBUG("on_socket_receive " << m_nodeInfo->socketAddress << " " << size);
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
		handleError(-1);
	}

private:
    void handleCommand_version(const MessageHeader& header, xul::data_input_stream& is)
    {
        VersionMessage msg;
        is >> msg;
        if (!is.good())
        {
			handleError(-1);
            return;
        }
        m_nodeInfo->nodeAddress.services = msg.services;
        m_nodeInfo->version = msg.version;
        m_nodeInfo->userAgent = msg.userAgent;
        XUL_INFO("handleCommand_version " << msg.version << " " << msg.userAgent);
        VerackMessage ackmsg;
		m_nodeInfo->messageSender->sendMessage(ackmsg);
    }
    void handleCommand_verack(const MessageHeader& header, xul::data_input_stream& is)
    {
        // make node connected
        m_nodeInfo->connected = true;
        m_handler->handleConnectionCreated(this);
    }
    void handleCommand_reject(const MessageHeader& header, xul::data_input_stream& is)
	{
		RejectMessage msg;
		is >> msg;
        if (!is.good())
        {
			handleError(-1);
            return;
        }
		XUL_WARN("handleCommand_reject " << msg.message << " " << (int)msg.code << " " << msg.reason << " " << msg.data.size());
		handleError(-2);
	}
    void handleError(int errcode)
    {
        assert(!m_nodeInfo->connected);
        if (!m_nodeInfo->inbound)
        {
            if (m_appInfo->getNodePool())
            {
                m_appInfo->getNodePool()->setPeerDisconnected(&m_nodeInfo->getPeerAddress(), errcode, false);
            }
        }

        close();
        if (m_handler)
            m_handler->removePendingNode(this);
        else
        {
            XUL_WARN("handleError no handler");
        }
    }
    void sendVersion()
    {
        XUL_DEBUG("sendVersion ");
        VersionMessage msg;
        msg.version = m_appInfo->getHostNodeInfo()->version;
        msg.services = m_appInfo->getHostNodeInfo()->nodeAddress.services;
        msg.timestamp = ::time(nullptr);
        msg.yourAddress.services = m_appInfo->getHostNodeInfo()->nodeAddress.services;
        msg.yourAddress.address = m_nodeInfo->nodeAddress.address;
        msg.myAddress.services = msg.services;
        msg.userAgent = m_appInfo->getHostNodeInfo()->userAgent;
        msg.nonce = xul::random::next_qword();
		// msg.nonce = 0xb294b9071f019ecc;
		// msg.userAgent = "/Satoshi:0.16.99/";
        msg.startHeight = 0;
		// msg.yourAddress.address.ip = 0x2d3f4b29;
		// msg.timestamp = 0x5ab71471;
		msg.relay = false;
		m_nodeInfo->messageSender->sendMessage(msg);
	}
	void receivePacket()
	{
        m_nodeInfo->socket->receive(20 * 1024);
	}
private:
	XUL_LOGGER_DEFINE();

	boost::intrusive_ptr<NodeInfo> m_nodeInfo;
	boost::intrusive_ptr<AppInfo> m_appInfo;
    PendingNodeHandler* m_handler;
	xul::time_counter m_startTime;
	bool m_closed;
};


PendingNode* createPendingNode(NodeInfo* nodeInfo, AppInfo* appInfo)
{
	return new PendingNodeImpl(nodeInfo, appInfo);
}


}
