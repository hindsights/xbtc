
#include "BlockIndexDB.hpp"
#include "ObjectDB.hpp"
#include "AppInfo.hpp"
#include "db.hpp"
#include "AppConfig.hpp"
#include "data/Block.hpp"
#include "util/serialization.hpp"

#include <leveldb/options.h>
#include <leveldb/cache.h>
#include <leveldb/filter_policy.h>
#include <leveldb/db.h>

#include <xul/lang/object_impl.hpp>
#include <xul/log/log.hpp>
#include <xul/os/paths.hpp>
#include <xul/data/date_time.hpp>
#include <xul/os/file_system.hpp>

#include <functional>
#include <memory>


namespace xbtc {


leveldb::Options getDBOptions(size_t nCacheSize)
{
    leveldb::Options options;
    options.block_cache = leveldb::NewLRUCache(nCacheSize / 2);
    options.write_buffer_size = nCacheSize / 4; // up to two write buffers may be held in memory simultaneously
    options.filter_policy = leveldb::NewBloomFilterPolicy(10);
    options.compression = leveldb::kNoCompression;
    options.max_open_files = 64;
    if (leveldb::kMajorVersion > 1 || (leveldb::kMajorVersion == 1 && leveldb::kMinorVersion >= 16))
    {
        // LevelDB versions before 1.16 consider short writes to be corruption. Only trigger error
        // on corruption in later versions.
        options.paranoid_checks = true;
    }
    return options;
}

class BlockIndexDBImpl : public xul::object_impl<BlockIndexDB>
{
public:
    explicit BlockIndexDBImpl(const AppConfig* config) : m_config(config), m_dataEncoding(xul::data_encoding::little_endian())
    {
        XUL_LOGGER_INIT("BlockIndexDB");
        XUL_REL_EVENT("new");
        m_db = std::make_shared<ObjectDB>(getDBOptions(m_config->dbCache), false);
    }
    ~BlockIndexDBImpl()
    {
        XUL_REL_EVENT("delete");
    }
    virtual bool open()
    {
        XUL_REL_EVENT("load " << m_config->dataDir);
        std::string dbdir = xul::paths::join(m_config->dataDir, "blocks/index");
        if (!xul::file_system::ensure_directory_exists(dbdir.c_str()))
        {
            XUL_REL_ERROR("failed to open db dir: " << dbdir);
            return false;
        }
        if (!m_db->open(dbdir.c_str()))
        {
            XUL_REL_ERROR("failed to open db " << dbdir);
            return false;
        }
        XUL_REL_EVENT("succeeded to open db " << dbdir);
        return true;
    }
    virtual bool writeBlocks(const BlockIndexesData& data)
    {
        ObjectDBWriteBatch batch = m_db->createWriteBatch();
        if (data.lastBlockFile > 0)
        {
            batch.write(DB_LAST_BLOCK, data.lastBlockFile);
        }
        for (const auto& item : *data.blocks)
        {
            batch.write(m_dataEncoding.encode(DB_BLOCK_INDEX, item.second->getHash()), *item.second);
        }
        for (const auto& fileinfo : data.files)
        {
            batch.write(m_dataEncoding.encode(DB_BLOCK_FILES, fileinfo.fileIndex), fileinfo);
        }
        return batch.execute(true);
    }
    virtual bool readLastBlockFile(int& fileIndex)
    {
        return m_db->read(DB_LAST_BLOCK, fileIndex);
    }
    virtual bool readBlockFileInfo(int fileIndex, BlockFileInfo& info)
    {
        std::string keystr = m_dataEncoding.encode(DB_BLOCK_FILES, fileIndex);
        XUL_DEBUG("readBlockFileInfo " << fileIndex << " " << xul::hex_encoding::upper_case().encode(keystr));
        return m_db->read(keystr, info);
    }
    virtual void loadAll(BlockIndexesData& data)
    {
        int lastBlockFile = 0;
        if (m_db->read(DB_LAST_BLOCK, lastBlockFile))
        {
            data.lastBlockFile = lastBlockFile;
        }
        boost::intrusive_ptr<DBIterator> cursor(m_db->createIterator());
        cursor->seekToFirst();
        int count = 0;
        while (cursor->valid())
        {
            std::string key = cursor->getKey();
            assert(!key.empty());
            xul::slice val = cursor->getValue();
            if (key[0] == DB_BLOCK_INDEX)
            {
                BlockIndexPtr block = loadBlockIndex(key, val);
                if (block)
                {
                    (*data.blocks)[block->getHash()] = block;
                }
            }
            else if (key[0] == DB_BLOCK_FILES)
            {
                BlockFileInfo fileinfo;
                if (loadBlockFileInfo(fileinfo, key, val))
                {
                    data.files.push_back(fileinfo);
                }
            }
            else
            {
                //
                XUL_WARN("loadAll ignore key " << xul::hex_encoding::upper_case().encode(key) << " " << count);
                cursor->next();
                continue;
            }
            ++count;
            cursor->next();
        }
    }
private:
    bool loadBlockFileInfo(BlockFileInfo& fileinfo, const std::string& key, const xul::slice& val)
    {
        char tag = 0;
        int fileIndex = 0;
        bool success = m_dataEncoding.decode(key, tag, fileIndex);
        if (!success)
        {
            XUL_WARN("loadBlockFileInfo invalid key " << xul::hex_encoding::upper_case().encode(key));
            return false;
        }
        success = m_dataEncoding.decode(val.data(), val.size(), fileinfo);
        if (!success)
        {
            XUL_WARN("loadBlockFileInfo invalid block " << xul::hex_encoding::upper_case().encode(key) << " " << val.size());
            return false;
        }
        fileinfo.fileIndex = fileIndex;
        return true;

    }
    BlockIndexPtr loadBlockIndex(const std::string& key, const xul::slice& val)
    {
        char tag;
        uint256 hash;
        bool success = m_dataEncoding.decode(key, tag, hash);
        if (!success)
        {
            XUL_WARN("loadBlockIndex invalid key " << xul::hex_encoding::upper_case().encode(key));
            return BlockIndexPtr();
        }
        assert(tag == DB_BLOCK_INDEX);
        BlockIndexPtr block = createBlockIndex();
        success = m_dataEncoding.decode(val.data(), val.size(), *block);
        if (!success)
        {
            XUL_WARN("loadBlockIndex invalid block " << xul::hex_encoding::upper_case().encode(key) << " " << val.size()
                     << " " << xul::unix_time::from_utc_time(block->header.timestamp));
            return BlockIndexPtr();
        }
        if (block->fileIndex > 10000)
        {
            assert(false);
            return BlockIndexPtr();
        }
        block->header.computeHash();
        if (block->height == 0)
        {
            XUL_WARN("loadAll genesis block " << xul::make_tuple(val.size(), block->height, block->fileIndex)
                     << " " << xul::unix_time::from_utc_time(block->header.timestamp));
        }
        if (block->getHash() != hash)
        {
            assert(false);
            return BlockIndexPtr();
        }
        return block;
    }
private:
    XUL_LOGGER_DEFINE();
    boost::intrusive_ptr<const AppConfig> m_config;
    std::shared_ptr<ObjectDB> m_db;
    xul::data_encoding m_dataEncoding;
};


BlockIndexDB* createBlockIndexDB(const AppConfig* config)
{
    return new BlockIndexDBImpl(config);
}


}
