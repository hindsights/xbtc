
#include "NodePool.hpp"
#include "PeerPoolImpl.hpp"
#include "NodeInfo.hpp"

#include <xul/lang/object_impl.hpp>
#include <xul/net/inet4_address.hpp>
#include <xul/net/inet_socket_address.hpp>
#include <xul/log/log.hpp>
#include <xul/util/time_counter.hpp>
#include <xul/std/maps.hpp>
#include <xul/macro/foreach.hpp>
#include <xul/io/structured_writer.hpp>
#include <xul/lang/object_ptr.hpp>
#include <xul/lang/object_impl.hpp>

#include <map>
#include <set>
#include <stdint.h>


namespace xbtc {


class NodePoolImpl : public PeerPoolImpl<PeerAddress>
{
public:
    explicit NodePoolImpl(const HostNodeInfo* hostNodeInfo) : m_hostNodeInfo(hostNodeInfo)
    {
    }

    virtual const PeerEndPointType& getLocalEndPoint() const
    {
        return m_hostNodeInfo->nodeAddress.address;
    }
    virtual bool isSelf(const PeerEndPointType& ep) const
    {
        return m_hostNodeInfo->nodeAddress.address == ep;
    }
    virtual bool isValidEndPoint(const PeerEndPointType& ep) const
    {
        return ep.isValid();
    }

private:
    boost::intrusive_ptr<const HostNodeInfo> m_hostNodeInfo;
};


NodePool* createNodePool(const HostNodeInfo* hostNodeInfo)
{
    return new NodePoolImpl(hostNodeInfo);
}


}
