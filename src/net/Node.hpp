#pragma once

#include "util/number.hpp"
#include <xul/lang/object.hpp>
#include <vector>
#include <ostream>

namespace xbtc {


class NodeInfo;
class NodeManager;
class NodeSyncInfo;
class BlockIndex;

class Node : public xul::object
{
public:
    virtual NodeInfo* getNodeInfo() = 0;
    virtual const NodeInfo* getNodeInfo() const = 0;
    virtual void start() = 0;
    virtual void close() = 0;
    virtual NodeSyncInfo& getSyncInfo() = 0;
    virtual void requestHeaders(std::vector<uint256>&& hashes) = 0;
    virtual void requestBlocks(const std::vector<BlockIndex*>& blocks) = 0;
};

Node* createNode(NodeInfo* nodeInfo, NodeManager& nodeManager);
std::ostream& operator<<(std::ostream& os, const Node& node);

}
