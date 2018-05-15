
#include "CoinDB.hpp"
#include "ObjectDB.hpp"
#include "AppInfo.hpp"
#include "db.hpp"
#include "AppConfig.hpp"
#include "data/Block.hpp"
#include "data/Coin.hpp"
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

#include <deque>
#include <functional>
#include <unordered_map>
#include <memory>


namespace xbtc {


extern leveldb::Options getDBOptions(size_t nCacheSize);

class CoinDBImpl : public xul::object_impl<CoinDB>
{
public:
    explicit CoinDBImpl(const AppConfig* config) : m_config(config), m_dataEncoding(xul::data_encoding::little_endian())
    {
        XUL_LOGGER_INIT("CoinDB");
        XUL_REL_EVENT("new");
        m_db = std::make_shared<ObjectDB>(getDBOptions(m_config->dbCache), true);
    }
    ~CoinDBImpl()
    {
        XUL_REL_EVENT("delete");
    }
    virtual bool open()
    {
        XUL_REL_EVENT("load " << m_config->dataDir);
        std::string dbdir = xul::paths::join(m_config->dataDir, "chainstate");
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
    virtual void loadAll(CoinsData& data)
    {
        uint256 bestBlockHash;
        if (m_db->read(DB_BEST_BLOCK, bestBlockHash))
        {
            data.bestBlockHash = bestBlockHash;
        }
        if (bestBlockHash.is_null())
        {
            boost::intrusive_ptr<DBIterator> cursor = m_db->createIterator();
            cursor->seekToFirst();
            while (cursor->valid())
            {
                std::string key = cursor->getKey();
                xul::slice value = cursor->getValue();
                XUL_DEBUG("loadAll key=" << xul::hex_encoding::lower_case().encode(key) << " " << value.size());
                if (key[0] == DB_COIN)
                {
                    Coin coin;
                    bool success = xul::data_encoding::little_endian().decode(value.data(), value.size(), coin);
                    assert(success);
                    assert(coin.height <= 0);
                }
                cursor->next();
            }
        }
    }
    virtual bool writeCoins(const CoinsData& data)
    {
        ObjectDBWriteBatch batch = m_db->createWriteBatch();
        for (const auto& item : data.addedCoins)
        {
            const TransactionOutPoint& out = item.first;
            const Coin& coin = item.second.coin;
            if (coin.output.value > 0 && item.second.dirty)
            {
                batch.write(m_dataEncoding.encode(DB_COIN, out.hash, out.index), coin);
            }
        }
        for (const auto& out : data.removedCoins)
        {
            batch.erase(m_dataEncoding.encode(DB_COIN, out.hash, out.index));
        }
        batch.write(DB_BEST_BLOCK, data.bestBlockHash);
        bool ret = batch.execute(true);
        XUL_EVENT("writeCoins " << data.bestBlockHash << " " << data.bestBlockHeight);
        return ret;
    }
    virtual bool readCoin(const TransactionOutPoint& out, Coin& coin)
    {
        std::string keystr = m_dataEncoding.encode(DB_COIN, out.hash, out.index);
        return m_db->read(keystr, coin);
    }
private:
private:
    XUL_LOGGER_DEFINE();
    boost::intrusive_ptr<const AppConfig> m_config;
    std::shared_ptr<ObjectDB> m_db;
    xul::data_encoding m_dataEncoding;
};


CoinDB* createCoinDB(const AppConfig* config)
{
    return new CoinDBImpl(config);
}


}
