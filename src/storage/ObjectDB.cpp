#include "ObjectDB.hpp"
#include "db.hpp"

#include <xul/lang/object_impl.hpp>
#include <xul/text/hex_encoding.hpp>


namespace xbtc {


const std::string OBFUSCATE_KEY_KEY("\x0E\x00obfuscate_key", 15);

class DataObfuscator : public xul::object_impl<xul::inplace_coding>
{
public:
    std::string key;

    explicit DataObfuscator(const std::string& k) : key(k) {}
    virtual void process(uint8_t* buf, int size)
    {
        transform(buf, buf, size);
    }
    virtual void transform(uint8_t* dst, const uint8_t* src, int size)
    {
        if (key.empty())
            return;
        for (size_t i = 0; i < size; ++i)
        {
            int j = i % key.size();
            dst[i] = src[i] ^ key[j];
        }
    }
};

ObjectDB::ObjectDB(const leveldb::Options& opts, bool needObfuscation) : m_obfuscationKey('\x00', OBFUSCATE_KEY_NUM_BYTES), m_needObfuscation(needObfuscation)
{
    XUL_LOGGER_INIT("ObjectDB");
    XUL_REL_EVENT("new");
    m_db = createLevelDB(opts);
    m_obfuscator = xul::create_object<xul::dummy_inplace_coding>();
}

ObjectDB::~ObjectDB()
{
    XUL_REL_EVENT("delete");
}

bool ObjectDB::open(const std::string& datadir)
{
    if (!m_db->open(datadir.c_str()))
    {
        XUL_REL_ERROR("failed to open database " << datadir);
        return false;
    }
    if (m_needObfuscation)
    {
        std::string obfuscationKey;
        LimitStringReader reader(obfuscationKey, 1024);
        if (read(OBFUSCATE_KEY_KEY, reader))
        {
            XUL_REL_INFO("read obfuscate key " << obfuscationKey.size() << " " << xul::hex_encoding::upper_case().encode(obfuscationKey));
            m_obfuscationKey = obfuscationKey;
            m_obfuscator = new DataObfuscator(m_obfuscationKey);
        }
    }
    return true;
}
void ObjectDB::close()
{
    
}
bool ObjectDB::readString(const std::string& key, std::string& val)
{
    if (!m_db->read(key, val))
        return false;
    m_obfuscator->process(val);
    return true;
}
bool ObjectDB::writeString(const std::string& key, const std::string& val, bool synced)
{
    if (!needObfuscate())
        return m_db->write(key, val, synced);
    std::string s = val;
    m_obfuscator->process(s);
    return m_db->write(key, s, synced);
}
bool ObjectDB::erase(const std::string& key, bool synced)
{
    return m_db->erase(key, synced);
}
bool ObjectDB::sync()
{
    return m_db->sync();
}
ObjectDBWriteBatch ObjectDB::createWriteBatch()
{
    return ObjectDBWriteBatch(*this, m_db->createWriteBatch());
}

void ObjectDBWriteBatch::writeString(const std::string& key, const std::string& val)
{
    if (m_db.needObfuscate())
    {
        std::string s = val;
        m_db.getObfuscator()->process(s);
        m_batch->write(key, s);
    }
    else
    {
        m_batch->write(key, val);
    }
}
void ObjectDBWriteBatch::erase(const std::string& key)
{
    m_batch->erase(key);
}
bool ObjectDBWriteBatch::execute(bool synced)
{
    return m_batch->execute(synced);
}

}
