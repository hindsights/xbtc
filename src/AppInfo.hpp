#pragma once

#include <xul/net/io_service.hpp>
#include <xul/lang/object_ptr.hpp>
#include <xul/lang/object.hpp>


namespace xul {
    class io_service;
}

namespace xbtc {


class HostNodeInfo;
class MessageEncoder;
class PeerAddress;
class AppConfig;
class BlockCache;
// class BlockStorage;
template <typename T>
class PeerPool;
typedef PeerPool<PeerAddress> NodePool;

class ThreadingInfo : public xul::object
{
public:
    boost::intrusive_ptr<xul::io_service> iosMain;
    boost::intrusive_ptr<xul::io_service> iosDisk;
};

class AppInfo : public xul::object
{
public:
    boost::intrusive_ptr<ThreadingInfo> threadingInfo;
    virtual xul::io_service* getIOService() = 0;
    virtual xul::io_service* getDiskIOService() = 0;
    virtual HostNodeInfo* getHostNodeInfo() = 0;
    virtual MessageEncoder* getMessageEncoder() = 0;
    virtual NodePool* getNodePool() = 0;
    virtual const AppConfig* getAppConfig() const = 0;
    virtual BlockCache* getBlockCache() = 0;
    // virtual BlockStorage* getBlockStorage() = 0;
};


}
