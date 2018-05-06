#pragma once

#include <xul/lang/object.hpp>
#include <stdint.h>


namespace xbtc{


class PeerAddress;
class PendingNode;
class NodeInfo;
class NodeConnector;
class AppInfo;
class MessageDecoder;


class PendingNodeHandler
{
public:
    virtual void handleConnectionCreated(PendingNode* node) = 0;
    virtual void handleConnectionError(PendingNode* node, int errcode) = 0;
    virtual void removePendingNode(PendingNode* node) = 0;
};


class PendingNode : public xul::object
{
public:
    virtual NodeInfo* getNodeInfo() = 0;
    virtual void setHandler(PendingNodeHandler *) = 0;
    virtual void close() = 0;
    virtual void onTick(int64_t times) = 0;
};


PendingNode* createPendingNode(NodeInfo* nodeInfo, AppInfo* appInfo);


}
