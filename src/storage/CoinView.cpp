#include "CoinView.hpp"
#include "CoinDB.hpp"
#include "AppConfig.hpp"
#include "ChainParams.hpp"
#include "data/Block.hpp"
#include "data/Coin.hpp"

#include <xul/lang/object_impl.hpp>
#include <xul/data/big_number_io.hpp>
#include <xul/data/date_time.hpp>
#include <xul/util/time_counter.hpp>
#include <xul/log/log.hpp>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <map>


namespace xbtc {


class CoinViewImpl : public xul::object_impl<CoinView>
{
public:
    explicit CoinViewImpl(const AppConfig* config) : m_config(config)
    {
        XUL_LOGGER_INIT("CoinView");
        XUL_REL_EVENT("new");
        m_db = createCoinDB(config);
    }
    ~CoinViewImpl()
    {
        XUL_REL_EVENT("delete");
    }

    virtual bool load()
    {
        XUL_DEBUG("load cache:");
        if (!m_db->open())
        {
            XUL_REL_ERROR("failed to open db:");
            return false;
        }
        m_db->loadAll(m_coinsData);
        return true;
    }
    virtual const uint256& getBestBlockHash() const
    {
        return m_coinsData.bestBlockHash;
    }
    virtual void setBestBlockHash(const uint256& hash, int height)
    {
        m_coinsData.bestBlockHash = hash;
        m_coinsData.bestBlockHeight = height;
    }
    virtual bool hasCoin(const TransactionOutPoint& out)
    {
        Coin* coin = fetchCoin(out);
        return coin != nullptr and coin->output.value > 0;
    }
    virtual void transfer(const Transaction* tx, int height)
    {
        XUL_DEBUG("transfer " << tx->getHash() << " " << xul::make_tuple(tx->inputs.size(), tx->outputs.size(), tx->isCoinBase()));
        if (tx->getHash().format(false) == "d5d27987d2a3dfc724e359870c6644b40e497bdc0589a033220fe15429d88599")
        {
            XUL_EVENT("transfer " << tx->getHash() << " " << xul::make_tuple(tx->inputs.size(), tx->outputs.size(), tx->isCoinBase()));
        }
        uint256 hash = tx->getHash();
        int64_t inputval = 0;
        if (!tx->isCoinBase())
        {
            for (const auto& input : tx->inputs)
            {
                Coin* coin = fetchCoin(input.previousOutput);
                assert(coin && coin->output.value > 0);
                if (input.previousOutput.hash.str() == "d5d27987d2a3dfc724e359870c6644b40e497bdc0589a033220fe15429d88599")
                {
                    XUL_EVENT("transfer test tx input: " << input.previousOutput.hash << " " << coin->output.value);
                }
                inputval += coin->output.value;
                removeCoin(input.previousOutput);
            }
        }
        int64_t outputval = 0;
        for (int i = 0; i < tx->outputs.size(); ++i)
        {
            TransactionOutPoint out(hash, i);
            outputval += tx->outputs[i].value;
            addCoin(out, Coin(tx->outputs[i], height, tx->isCoinBase()));
        }
        if (!tx->isCoinBase())
        {
            assert(inputval >= outputval);
        }
        flush();
    }
    virtual Coin* fetchCoin(const TransactionOutPoint& out)
    {
        if (m_coinsData.removedCoins.find(out) != m_coinsData.removedCoins.end())
            return nullptr;
        auto iter = m_coinsData.addedCoins.find(out);
        if (iter != m_coinsData.addedCoins.end())
        {
            return &iter->second.coin;
        }
        CoinEntry& entry = m_coinsData.addedCoins[out];
        Coin tempcoin;
        if (!m_db->readCoin(out, tempcoin))
        {
            // create null coin entry
            XUL_DEBUG("failed to read coin " << out.hash << " " << out.index);
            return &entry.coin;
        }
        assert(tempcoin.output.value > 0);
        entry.coin = tempcoin;
        return &entry.coin;
    }
private:
    void flush()
    {
        if (m_lastFlushTime.elapsed() < 5000)
            return;
        m_lastFlushTime.sync();
        m_db->writeCoins(m_coinsData);
        m_coinsData.addedCoins.clear();
        m_coinsData.removedCoins.clear();
    }
    void addCoin(const TransactionOutPoint& out, const Coin& coin)
    {
        // if (m_coinsData.addedCoins.find(out) != m_coinsData.addedCoins.end())
        // {
        //     auto iter = m_coinsData.addedCoins.find(out);
        //     auto outpoint = iter->first;
        //     const auto& entry = iter->second;
        //     XUL_EVENT("addCoin dup " << outpoint.hash << " " << outpoint.index << " " << xul::make_tuple(entry.coin.output.value, entry.coin.height)
        //               << " " << xul::make_tuple(coin.output.value, coin.height));
        //     assert(outpoint == out);
        //     assert(false);
        // }
        m_coinsData.addedCoins[out] = CoinEntry(coin, true);
    }
    void removeCoin(const TransactionOutPoint& out)
    {
        // assert(m_coinsData.addedCoins.find(out) != m_coinsData.addedCoins.end());
        m_coinsData.addedCoins.erase(out);
        m_coinsData.removedCoins.insert(out);
    }
private:
    XUL_LOGGER_DEFINE();
    boost::intrusive_ptr<const AppConfig> m_config;
    boost::intrusive_ptr<CoinDB> m_db;
    CoinsData m_coinsData;
//    CoinMap m_coins;
    CoinSet m_removedCoins;
    xul::time_counter m_lastFlushTime;
};


CoinView* createCoinView(const AppConfig* config)
{
    return new CoinViewImpl(config);
}


}
