
#include "MessageCodec.hpp"

#include "Message.hpp"
#include "util/Hasher.hpp"

#include <xul/lang/object_impl.hpp>
#include <xul/data/buffer.hpp>
#include <xul/io/data_output_stream.hpp>
#include <xul/log/log.hpp>
#include <xul/io/data_input_stream.hpp>

#include <boost/shared_ptr.hpp>


namespace xbtc {


inline bool writeMessageCommand(xul::data_output_stream& os, const std::string& msgtype)
{
    const int max_command_size = 12;
    if (msgtype.size() >= max_command_size) {
        assert(false);
        return false;
    }
    char buf[max_command_size];
    memset(buf, 0, max_command_size);
    memcpy(buf, msgtype.data(), msgtype.size());
    os.write_array(buf, max_command_size);
    return true;
}

class MessageEncoderImpl : public xul::object_impl<MessageEncoder>
{
public:
    explicit MessageEncoderImpl(uint32_t protocolMagic) : m_protocolMagic(protocolMagic)
    {
        XUL_LOGGER_INIT("MessageEncoder");
        XUL_DEBUG("new");
        m_buffer.resize(1*1024*1024);
    }

    virtual const uint8_t* getData() const
    {
        return m_buffer.data();
    }
    virtual int encode(const Message& msg)
    {
        xul::memory_data_output_stream os(m_buffer.data(), m_buffer.size(), false);
        os.write_uint32(m_protocolMagic);
        if (!writeMessageCommand(os, msg.getMessageType()))
        {
            return -1;
        }
        int lenthPos = os.position();
        // length will be written here later
        os.write_uint32(0);
        // checksum will be written here later
        os.write_uint32(0);
        int payloadPos = os.position();
        msg.write_object(os);
        int totalSize = os.position();
        int payloadSize = totalSize - payloadPos;
        os.seek(lenthPos);
        os.write_uint32(payloadSize);
        uint256 checksum = Hasher256::hash(m_buffer.data() + payloadPos, payloadSize);
        os.write_bytes(&checksum[0], 4);
        os.seek(totalSize);
        return totalSize;
    }

protected:
    XUL_LOGGER_DEFINE();
    xul::byte_buffer m_buffer;
    const uint32_t m_protocolMagic;
};

class CheckedMessageDecoderListener : public MessageDecoderListener
{
public:
    virtual void onMessageDecoded(const MessageHeader& header, const std::vector<uint8_t>& payload)
    {
//        assert(false);
    }
    static CheckedMessageDecoderListener* instance()
    {
        static CheckedMessageDecoderListener dummyListener;
        return &dummyListener;
    }
};

class MessageDecoderImpl : public xul::object_impl<MessageDecoder>
{
public:
    explicit MessageDecoderImpl(uint32_t protocolMagic) : m_protocolMagic(protocolMagic)
    {
        setListener(nullptr);
    }
    virtual bool decode(const uint8_t* data, int size)
    {
        XUL_APP_DEBUG("MessageDecoder.decode buffer " << xul::make_tuple(m_buffer.size(), size));
        const uint8_t* buf = data;
        int bufsize = size;
        if (!m_buffer.empty())
        {
            m_buffer.insert(m_buffer.end(), data, data + size);
            buf = m_buffer.data();
            bufsize = m_buffer.size();
            XUL_APP_DEBUG("MessageDecoder.decode append to buffer " << xul::make_tuple(m_buffer.size(), size));
        }
        int processedSize = decodeMessages(buf, bufsize);
        if (processedSize < 0)
            return false;
        assert(processedSize <= bufsize);
        std::vector<uint8_t> remaining(buf + processedSize, buf + bufsize);
        XUL_APP_DEBUG("MessageDecoder.decode remaining " << xul::make_tuple(m_buffer.size(), size) << " " << xul::make_tuple(processedSize, bufsize, remaining.size()));
        m_buffer.swap(remaining);
        return true;
    }
    virtual void setListener(MessageDecoderListener* listener)
    {
        m_listener = listener ? listener : CheckedMessageDecoderListener::instance();
    }

private:
    int decodeMessages(const uint8_t* data, int size)
    {
        int decodedSize = 0;
        while (decodedSize < size)
        {
            int msgsize = decodeOneMessage(data + decodedSize, size - decodedSize);
            if (msgsize < 0)
                return msgsize;
            if (msgsize == 0)
                break;
            decodedSize += msgsize;
        }
        return decodedSize;
    }
    int decodeOneMessage(const uint8_t* data, int size)
    {
        if (size < PROTOCOL_HEADER_SIZE)
        {
//            assert(false);
            return 0;
        }
        MessageHeader header;
        xul::memory_data_input_stream is(data, size, false);
        if (!header.read_object(is))
        {
            assert(false);
            return 0;
        }
        if (header.magic != m_protocolMagic)
        {
            assert(false);
            is.set_bad();
            return -1;
        }
        if (size < header.length + PROTOCOL_HEADER_SIZE)
        {
            return 0;
        }
        const uint8_t* payloadData = data + PROTOCOL_HEADER_SIZE;
        uint256 checksum = Hasher256::hash(payloadData, header.length);
        uint32_t checksumVal = xul::bit_converter::little_endian().to_dword(checksum.data());
        if (checksumVal != header.checksum)
        {
            assert(false);
            is.set_bad();
            return -2;
        }
        std::vector<uint8_t> payload;
        payload.assign(payloadData, payloadData + header.length);
        m_listener->onMessageDecoded(header, payload);
        return header.length + PROTOCOL_HEADER_SIZE;
    }

private:
    xul::openssl_sha256_hasher m_hasher;
    MessageDecoderListener* m_listener;
    std::vector<uint8_t> m_buffer;
    const uint32_t m_protocolMagic;
};

MessageEncoder* createMessageEncoder(uint32_t protocolMagic)
{
    return new MessageEncoderImpl(protocolMagic);
}

MessageDecoder* createMessageDecoder(uint32_t protocolMagic)
{
    return new MessageDecoderImpl(protocolMagic);
}

}
