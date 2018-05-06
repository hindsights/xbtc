#pragma once

#include "util/DB.hpp"
#include "util/serialization.hpp"
#include <xul/lang/object_ptr.hpp>
#include <xul/io/data_encoding.hpp>
#include <xul/crypto/inplace_coding.hpp>
#include <xul/log/log.hpp>
#include <boost/noncopyable.hpp>
#include <sstream>


namespace xbtc {


class ObjectDBWriteBatch;

class ObjectDB : private boost::noncopyable
{
public:
    explicit ObjectDB(const leveldb::Options& opts, bool needObfuscation);
    ~ObjectDB();
    bool open(const std::string& datadir);
    void close();
    bool readString(const std::string& key, std::string& val);
    bool writeString(const std::string& key, const std::string& val, bool synced = false);
    bool erase(const std::string& key, bool synced = false);
    bool sync();
    template <typename T>
    bool read(const std::string& key, T& obj)
    {
        std::string s;
        if (!this->readString(key, s))
            return false;
        char dummybuf[1];
        xul::memory_data_input_stream is(s.empty() ? dummybuf : s.data(), s.size(), false);
        is >> obj;
        return is.good();
    }
    template <typename T>
    bool write(const std::string& key, const T& obj)
    {
        std::string s = xul::data_encoding::little_endian().encode(obj);
        return this->writeString(key, s);
    }
    DBIterator* createIterator()
    {
        return m_db->createIterator();
    }
    ObjectDBWriteBatch createWriteBatch();

    bool needObfuscate() const { return m_needObfuscation; }
    xul::inplace_coding* getObfuscator() { return m_obfuscator.get(); }

private:
    XUL_LOGGER_DEFINE();
    boost::intrusive_ptr<DB> m_db;
    std::string m_obfuscationKey;
    const bool m_needObfuscation;
    boost::intrusive_ptr<xul::inplace_coding> m_obfuscator;
};


class ObjectDBWriteBatch
{
public:
    explicit ObjectDBWriteBatch(ObjectDB& db, DBWriteBatch* batch) : m_db(db), m_batch(batch) { }
    ~ObjectDBWriteBatch() {}
    template <typename T>
    void write(const std::string& key, const T& obj)
    {
        std::string s = xul::data_encoding::little_endian().encode(obj);
        this->writeString(key, s);
    }
    void writeString(const std::string& key, const std::string& val);
    void erase(const std::string& key);
    bool execute(bool synced);
private:
    ObjectDB& m_db;
    boost::intrusive_ptr<DBWriteBatch> m_batch;
};


}
