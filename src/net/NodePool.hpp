#pragma once

#include "PeerPool.hpp"

#include <xul/lang/object.hpp>
#include <xul/util/dumpable.hpp>
#include <stdint.h>


namespace xbtc {


class PeerAddress;
class HostNodeInfo;
typedef PeerPool<PeerAddress> NodePool;

NodePool* createNodePool(const HostNodeInfo* hostNodeInfo);


}
