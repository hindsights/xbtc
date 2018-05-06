#pragma once

#include <xul/lang/object.hpp>
#include <xul/util/dumpable.hpp>
#include <stdint.h>


namespace xbtc {


template <typename PeerEndPointT>
class PeerPool : public xul::object
{
public:
    virtual void dump(xul::structured_writer* writer, int level) const = 0;
    virtual void addPeer(const PeerEndPointT* peer) = 0;
    virtual void addPeers(const PeerEndPointT* peers, int count) = 0;
    virtual bool getConnectionPeer(PeerEndPointT* peer) = 0;
    virtual void setPeerConnecting(const PeerEndPointT* peer) = 0;
    virtual void setPeerConnected(const PeerEndPointT* peer, int rtt) = 0;
    virtual void setPeerDisconnected(const PeerEndPointT* peer, int errcode, bool connected) = 0;
    virtual void onTick(int64_t times) = 0;
    virtual int getPeerCount() = 0;
    virtual int getConnectPeerCount() = 0;
};


}
