
#include "MessageSender.hpp"

#include "MessageCodec.hpp"
#include "Message.hpp"
#include "NodeInfo.hpp"

#include <xul/lang/object_impl.hpp>
#include <xul/net/tcp_socket.hpp>
#include <xul/net/inet_socket_address.hpp>
#include <xul/log/log.hpp>


namespace xbtc {


class MessageSenderImpl : public xul::object_impl<MessageSender>
{
public:
    explicit MessageSenderImpl(NodeInfo* nodeInfo, MessageEncoder* msgEncoder)
        : m_nodeInfo(nodeInfo)
        , m_messageEncoder(msgEncoder)
    {
        XUL_LOGGER_INIT("MessageSender");
        XUL_DEBUG("new");
    }
    virtual ~MessageSenderImpl()
    {
        XUL_DEBUG("delete");
    }

    virtual bool sendMessage(const Message& msg)
    {
        XUL_DEBUG("sendMessage " << msg.getMessageType());
        int size = m_messageEncoder->encode(msg);
        return m_nodeInfo->socket->send(m_messageEncoder->getData(), size);
    }

private:
    XUL_LOGGER_DEFINE();

    boost::intrusive_ptr<NodeInfo> m_nodeInfo;
    boost::intrusive_ptr<MessageEncoder> m_messageEncoder;
};


MessageSender* createMessageSender(NodeInfo* nodeInfo, MessageEncoder* msgEncoder)
{
    return new MessageSenderImpl(nodeInfo, msgEncoder);
}


}
