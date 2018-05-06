#include "DB.hpp"

#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <leveldb/options.h>
#include <leveldb/env.h>

#include <xul/lang/object_impl.hpp>
#include <xul/text/hex_encoding.hpp>
#include <xul/log/log.hpp>


namespace xbtc {


class LevelDBImpl : public xul::object_impl<DB>
{
public:
    explicit LevelDBImpl(const leveldb::Options& opts) : m_options(opts)
    {
        XUL_LOGGER_INIT("LevelDB");
        XUL_REL_INFO("new");
        m_readOptions.verify_checksums = true;
        m_iterOptions.verify_checksums = true;
        m_iterOptions.fill_cache = false;
        m_syncOptions.sync = true;
        m_options.create_if_missing = true;
    }
    ~LevelDBImpl()
    {
        XUL_REL_INFO("delete");
    }

    virtual bool open(const char* datadir)
    {
        leveldb::DB* pdb = nullptr;
        auto status = leveldb::DB::Open(m_options, datadir, &pdb);
        if (status.ok())
        {
            XUL_REL_INFO("succeeded to open db: " << datadir << " " << status.ToString());
            m_db.reset(pdb);
            return true;
        }
        XUL_REL_INFO("failed to open db: " << datadir << " " << status.ToString());
        return false;
    }
    virtual void close()
    {
    }
    virtual bool read(const std::string& key, std::string& val)
    {
        auto status = m_db->Get(m_readOptions, key, &val);
        if (status.ok())
            return true;
        XUL_REL_INFO("failed to read: " << xul::hex_encoding::lower_case().encode(key) << " " << status.ToString());
        return false;
    }
    virtual bool write(const std::string& key, const std::string& val, bool synced)
    {
        auto status = m_db->Put(synced ? m_syncOptions : m_writeOptions, key, val);
        if (status.ok())
            return true;
        XUL_REL_INFO("failed to write: " << xul::hex_encoding::lower_case().encode(key) << " " << val.size() << " " << status.ToString());
        return false;
    }
    virtual bool erase(const std::string& key, bool synced)
    {
        auto status = m_db->Delete(synced ? m_syncOptions : m_writeOptions, key);
        if (status.ok())
            return true;
        XUL_REL_INFO("failed to erase: " << xul::hex_encoding::lower_case().encode(key) << " " << status.ToString());
        return false;
    }
    virtual bool sync()
    {
        boost::intrusive_ptr<DBWriteBatch> batch(createWriteBatch());
        return batch->execute(true);
    }
    virtual DBWriteBatch* createWriteBatch();
    virtual DBIterator* createIterator();
    leveldb::DB* native() { return m_db.get(); }
    bool execute(leveldb::WriteBatch& batch, bool synced)
    {
        auto status = m_db->Write(synced ? m_syncOptions : m_writeOptions, &batch);
        if (status.ok())
            return true;
        XUL_REL_INFO("failed to execute: " << status.ToString());
        return false;
    }
private:
    friend class LevelDBWriteBatchImpl;
    XUL_LOGGER_DEFINE();
    std::unique_ptr<leveldb::Env> m_env;
    leveldb::Options m_options;
    leveldb::ReadOptions m_readOptions;
    leveldb::ReadOptions m_iterOptions;
    leveldb::WriteOptions m_writeOptions;
    leveldb::WriteOptions m_syncOptions;
    std::unique_ptr<leveldb::DB> m_db;
};

class LevelDBWriteBatchImpl : public xul::object_impl<DBWriteBatch>
{
public:
    explicit LevelDBWriteBatchImpl(LevelDBImpl& db) : m_db(db)
    {

    }
    virtual void clear()
    {
        m_batch.Clear();
    }
    virtual void write(const std::string& key, const std::string& val)
    {
        m_batch.Put(key, val);
    }
    virtual void erase(const std::string& key)
    {
        m_batch.Delete(key);
    }
    virtual bool execute(bool synced)
    {
        return m_db.execute(m_batch, synced);
    }
private:
    LevelDBImpl& m_db;
    leveldb::WriteBatch m_batch;
};

class LevelDBIteratorImpl : public xul::object_impl<DBIterator>
{
public:
    explicit LevelDBIteratorImpl(LevelDBImpl& db, leveldb::Iterator* iter) : m_db(db), m_iterator(iter)
    {
    }
    virtual bool valid() const
    {
        return m_iterator->Valid();
    }
    virtual std::string getKey() const
    {
        return m_iterator->key().ToString();
    }
    virtual xul::slice getValue() const
    {
        leveldb::Slice val = m_iterator->value();
        return xul::slice(val.data(), val.size());
    }
    virtual void seek(const std::string& key)
    {
        m_iterator->Seek(key);
    }
    virtual void seekToFirst()
    {
        m_iterator->SeekToFirst();
    }
    virtual void seekToLast()
    {
        m_iterator->SeekToLast();
    }
    virtual void next()
    {
        m_iterator->Next();
    }
    virtual void prev()
    {
        m_iterator->Prev();
    }
private:
    LevelDBImpl& m_db;
    std::unique_ptr<leveldb::Iterator> m_iterator;
};

DBWriteBatch* LevelDBImpl::createWriteBatch()
{
    return new LevelDBWriteBatchImpl(*this);
}

DBIterator* LevelDBImpl::createIterator()
{
    return new LevelDBIteratorImpl(*this, m_db->NewIterator(m_iterOptions));
}

DB* createLevelDB(const leveldb::Options& opts)
{
    return new LevelDBImpl(opts);
}


}
