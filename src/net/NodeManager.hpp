#pragma once

#include "AppInfo.hpp"
#include <xul/lang/object.hpp>
#include <stdint.h>

namespace xul {
    class structured_writer;
}

namespace xbtc {


class PeerConnection;
class PeerAddress;
class NodeInfo;
class AppInfo;
class Node;
class BlockSynchronizer;

class NodeManager : public xul::object
{
public:
    virtual void start() = 0;
    virtual void onTick(int64_t times) = 0;
    virtual void schedule() = 0;
    virtual AppInfo* getAppInfo() = 0;
    virtual int getNodeShortage() const = 0;
    virtual Node* addNewNode(NodeInfo* nodeInfo) = 0;
    virtual void removeNode(Node* node) = 0;
    virtual bool doesNodeAddressConnected(const PeerAddress&) = 0;
    virtual void stop() = 0;
    virtual BlockSynchronizer* getBlockSynchronizer() = 0;
};


NodeManager* createNodeManager(AppInfo* appInfo);


}
