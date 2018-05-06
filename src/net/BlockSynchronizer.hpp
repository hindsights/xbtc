#pragma once

#include <xul/lang/object.hpp>
#include <vector>

namespace xbtc {


class NodeManager;
class BlockHeader;
class Block;
class Node;

class BlockSynchronizer : public xul::object
{
public:
    virtual void onTick(int64_t times) = 0;
    virtual void addNode(Node* node) = 0;
    virtual void removeNode(Node* node) = 0;
    virtual void handleHeaders(std::vector<BlockHeader>& headers, Node* node) = 0;
    virtual void handleBlock(Block* block, Node* node) = 0;
};

BlockSynchronizer* createBlockSynchronizer(NodeManager& nodeManager);


}
