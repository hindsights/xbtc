#pragma once

#include "PeerPool.hpp"

#include <xul/lang/object_impl.hpp>
#include <xul/net/inet4_address.hpp>
#include <xul/net/inet_socket_address.hpp>
#include <xul/log/log.hpp>
#include <xul/util/time_counter.hpp>
#include <xul/std/maps.hpp>
#include <xul/macro/foreach.hpp>
#include <xul/io/structured_writer.hpp>

#include <map>
#include <set>
#include <stdint.h>


namespace xbtc {


class PoolPeerState
{
public:
    enum Enum
    {
        standby = 0,
        connecting = 1,
        connected = 2,
    };
};


template <typename PeerEndPointT>
class PoolPeerInfo : public xul::object_impl<xul::object>
{
public:
    PeerEndPointT endpoint;
    int rtt;

    xul::time_counter creationTime;
    xul::time_counter lastConnectTime;
    xul::time_counter lastActiveTime;

    //bool connected;
    PoolPeerState::Enum state;
    int disconnectError;

    explicit PoolPeerInfo(const PeerEndPointT& ep)
        : endpoint(ep)
        , lastConnectTime(0)
        , state(PoolPeerState::standby)
        , disconnectError(0)
        , rtt(-1)
    {
    }

    bool isStandby() const
    {
        return PoolPeerState::standby == state;
    }
    bool isConnected() const
    {
        return PoolPeerState::connected == state;
    }
};


template <typename PeerEndPointT>
class PeerPoolImpl : public xul::object_impl<PeerPool<PeerEndPointT> >
{
public:
    typedef PoolPeerInfo<PeerEndPointT> PoolPeerInfoType;
    typedef PeerEndPointT PeerEndPointType;
    typedef boost::intrusive_ptr<PoolPeerInfoType> PoolPeerInfoPtr;
    typedef std::map<PeerEndPointType, PoolPeerInfoPtr> PoolPeerInfoMap;
    typedef std::multimap<int64_t, PoolPeerInfoPtr> TimeIndexedPoolPeerInfoMap;

    class PoolItemUpdater
    {
    private:
        PeerPoolImpl& m_pool;
        const PoolPeerInfoPtr& m_item;
    public:
        explicit PoolItemUpdater(PeerPoolImpl* pool, const PoolPeerInfoPtr& item) : m_pool(*pool), m_item(item)
        {
            m_pool.removePeerMap(m_item);
        }
        ~PoolItemUpdater()
        {
            m_pool.addPeerMap(m_item);
        }
    };

    explicit PeerPoolImpl()
    {
        XUL_LOGGER_INIT("PeerPool");
        XUL_DEBUG("new");
    }
    virtual ~PeerPoolImpl()
    {
        XUL_DEBUG("delete");
    }

    virtual const PeerEndPointType& getLocalEndPoint() const = 0;
    virtual bool isSelf(const PeerEndPointType& ep) const = 0;
    virtual bool isValidEndPoint(const PeerEndPointType& ep) const = 0;

    virtual int   getPeerCount()
    {
        return m_peers.size();
    }

    virtual int   getConnectPeerCount()
    {
        return m_connectionPool.size();
    }
    virtual void dump(xul::structured_writer* writer, int level) const
    {
        writer->write_integer("totalPeerCount", m_peers.size());
        writer->write_integer("connectionPoolSize", m_connectionPool.size());
        writer->write_integer("expiringPoolSize", m_expiringPool.size());
    }

    virtual void addPeer(const PeerEndPointType* peer)
    {
        assert(peer);
        doAddPeer(*peer);
    }
    virtual void addPeers(const PeerEndPointType* peers, int count)
    {
        XUL_DEBUG("addPeers " << count);
        for (int i = 0; i < count; ++i)
        {
            const PeerEndPointType& entry = peers[i];
            doAddPeer(entry);
        }
    }
    virtual bool getConnectionPeer(PeerEndPointType* peer)
    {
        XUL_DEBUG("getConnectionPeer");
        assert(peer);
        if (m_connectionPool.empty())
            return false;
        PoolPeerInfoPtr item = m_connectionPool.begin()->second;
        assert(isValidEndPoint(item->endpoint));
        if (item->lastConnectTime.elapsed32() < 5000)
            return false;

        assert(item->isStandby());
        PoolItemUpdater updater(this, item);
        item->lastConnectTime.sync();
        *peer = item->endpoint;
        XUL_DEBUG("getConnectionPeer peer " << item->endpoint);
        return true;
    }
    virtual void setPeerConnecting(const PeerEndPointType* peer)
    {
        XUL_DEBUG("setPeerConnecting " << *peer);
        assert(this->isValidEndPoint(*peer));
        PoolPeerInfoPtr item = findPeer(*peer);
        PoolItemUpdater updater(this, item);
        item->state = PoolPeerState::connecting;
    }
    virtual void setPeerConnected(const PeerEndPointType* peer, int rtt)
    {
        XUL_DEBUG("setPeerConnected " << *peer);
        if (!this->isValidEndPoint(*peer))
            return;
        PoolPeerInfoPtr item = findPeer(*peer);
        PoolItemUpdater updater(this, item);
        item->state = PoolPeerState::connected;
        item->rtt = rtt;
    }
    virtual void setPeerDisconnected(const PeerEndPointType* peer, int errcode, bool connected)
    {
        XUL_DEBUG("setPeerDisconnected " << *peer << xul::make_tuple(errcode, connected));
        if (!this->isValidEndPoint(*peer))
            return;
        PoolPeerInfoPtr item = findPeer(*peer);
        PoolItemUpdater updater(this, item);
        item->state = PoolPeerState::standby;
        item->disconnectError = errcode;
        if (connected)
        {
            item->lastActiveTime.sync();
        }
    }

    virtual void onTick(int64_t times)
    {
        if (times % 20 == 0)
        {
            removeExpired();
        }
    }

private:
    void removeExpired()
    {
        int erasedCount = 0;
        while (!m_expiringPool.empty())
        {
            PoolPeerInfoPtr ptr = m_expiringPool.begin()->second;
            if (ptr.get()->lastActiveTime.elapsed32() > (5 * 60 * 1000))
            {
                XUL_DEBUG("removeExpired " << ptr->endpoint);
                m_peers.erase(ptr->endpoint);
                removePeerMap(ptr);
                ++erasedCount;
            }
            else
            {
                break;
            }
        }

        if (m_peers.size() < m_connectionPool.size() || m_expiringPool.size() > m_peers.size())
        {
            assert(false);
        }

        XUL_DEBUG("removeExpired " << xul::make_tuple(erasedCount, m_peers.size(), m_connectionPool.size(), m_expiringPool.size()));
    }

    PoolPeerInfoPtr findPeer(const PeerEndPointType& peer)
    {
        assert(this->isValidEndPoint(peer));
        PoolPeerInfoPtr& item = m_peers[peer];
        if (!item)
        {
            item = new PoolPeerInfoType(peer);
            addPeerMap(item);
        }

        return item;
    }
    void doAddPeer(const PeerEndPointType& peer)
    {
        const PeerEndPointType& addr = peer;
        XUL_DEBUG("add peer " << addr);
        if (this->isSelf(addr))
            return;
        PoolPeerInfoPtr& item = m_peers[addr];
        if (!item)
        {
            XUL_DEBUG("add new peer " << addr);
            item = new PoolPeerInfoType(addr);
        }
        else
        {
            removePeerMap(item);
        }

        item->lastActiveTime.sync();
        addPeerMap(item);
    }
    void removePeerMap(const PoolPeerInfoPtr& item)
    {
        xul::multimaps::erase(m_expiringPool, item->lastActiveTime.get_count(), item);
        xul::multimaps::erase(m_connectionPool, item->lastConnectTime.get_count(), item);
    }
    void addPeerMap(const PoolPeerInfoPtr& item)
    {
        if (item->isStandby())
            m_connectionPool.insert(std::make_pair(item->lastConnectTime.get_count(), item));

        if (!item->isConnected())
            m_expiringPool.insert(std::make_pair(item->lastActiveTime.get_count(), item));
    }

private:
    XUL_LOGGER_DEFINE();
    PoolPeerInfoMap m_peers;
    TimeIndexedPoolPeerInfoMap m_connectionPool;
    TimeIndexedPoolPeerInfoMap m_expiringPool;
};


}
