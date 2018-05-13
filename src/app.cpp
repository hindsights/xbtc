#include "app.hpp"
#include "AppConfig.hpp"
#include "AppInfo.hpp"
#include "net/NodeInfo.hpp"
#include "net/MessageCodec.hpp"
#include "net/NodePool.hpp"
#include "net/NodeManager.hpp"
#include "net/NodeAddress.hpp"
#include "version.hpp"
#include "flags.hpp"
#include "storage/BlockCache.hpp"
#include "storage/BlockStorage.hpp"
#include "data/Block.hpp"
#include "script/Script.hpp"
#include "data/MerkleTree.hpp"
#include "storage/ChainParams.hpp"

#include <xul/io/data_input_stream.hpp>
#include <xul/io/data_output_stream.hpp>
#include <xul/io/data_encoding.hpp>
#include <xul/net/inet_socket_address.hpp>
#include <xul/lang/object_impl.hpp>
#include <xul/lang/object_base.hpp>
#include <xul/net/io_service.hpp>
#include <xul/net/inet4_address.hpp>
#include <xul/log/log.hpp>
#include <xul/log/log_manager.hpp>
#include <xul/std/strings.hpp>
#include <xul/util/random.hpp>
#include <xul/data/big_number_io.hpp>
#include <xul/util/data_parser.hpp>
#include <xul/util/options_wrapper.hpp>
#include <xul/util/timer_holder.hpp>
#include <time.h>

namespace xbtc {


const std::string CLIENT_NAME("xbtc");

class AppInfoImpl : public xul::object_impl<AppInfo>
{
public:
    boost::intrusive_ptr<HostNodeInfo> hostNodeInfo;
    boost::intrusive_ptr<MessageEncoder> messageEncoder;
    boost::intrusive_ptr<const AppConfig> appConfig;
    boost::intrusive_ptr<NodePool> nodePool;
    boost::intrusive_ptr<BlockCache> blockCache;
    boost::intrusive_ptr<const ChainParams> chainParams;

    AppInfoImpl()
    {
        threadingInfo = xul::create_object<ThreadingInfo>();
    }

    virtual xul::io_service* getIOService() { return threadingInfo->iosMain.get(); }
    virtual xul::io_service* getDiskIOService() { return threadingInfo->iosDisk.get(); }
    virtual HostNodeInfo* getHostNodeInfo() { return hostNodeInfo.get(); }
    virtual MessageEncoder* getMessageEncoder() { return messageEncoder.get(); }
    virtual NodePool* getNodePool() { return nodePool.get(); }
    virtual const AppConfig* getAppConfig() const { return appConfig.get(); }
    virtual BlockCache* getBlockCache() { return blockCache.get(); }
    virtual const ChainParams* getChainParams() const { return chainParams.get(); }
};

class BitCoinAppImpl : public xul::object_impl<BitCoinApp>, public xul::timer_listener
{
public:
    BitCoinAppImpl()
    {
        xul::log_manager::start_console_log_service("xbtc");
        xul::random::init_seed(time(nullptr));
        m_appInfo = new AppInfoImpl;
        m_appInfo->threadingInfo->iosDisk = xul::create_io_service();
        m_appInfo->threadingInfo->iosMain = xul::create_io_service();
        m_appInfo->hostNodeInfo = createHostNodeInfo();
        m_appInfo->hostNodeInfo->userAgent = formatUserAgent(CLIENT_NAME, CLIENT_VERSION, std::vector<std::string>());
        m_appInfo->hostNodeInfo->nodeAddress.services = ServiceFlags::NODE_NETWORK;
        m_appInfo->hostNodeInfo->version = PROTOCOL_VERSION;
        m_appInfo->nodePool = createNodePool(m_appInfo->getHostNodeInfo());
        m_nodeManager = createNodeManager(m_appInfo.get());
        m_timer.create_periodic_timer(m_appInfo->getIOService());
        m_timer.set_listener(this);
    }

    bool start(const AppConfig* config)
    {
        m_config = config;
        m_appInfo->appConfig = config;
        m_appInfo->chainParams = config->testNet ? createTestNetChainParams() : createMainChainParams();
        m_appInfo->messageEncoder = createMessageEncoder(m_appInfo->chainParams->protocolMagic);
        BlockStorage* blockStorage = createBlockStorage(m_appInfo.get());
        m_appInfo->blockCache = createBlockCache(config, blockStorage, m_appInfo->chainParams.get());
        // first load data from storage into cache, then start background io services
        m_appInfo->blockCache->load();
        m_appInfo->threadingInfo->iosMain->start();
        m_appInfo->threadingInfo->iosDisk->start();
        m_nodeManager->start();
        m_timer.start(1000);
        return true;
    }
    void stop()
    {
    }
    void wait()
    {
        m_appInfo->threadingInfo->iosMain->wait();
    }

    virtual void on_timer_elapsed(xul::timer* sender)
    {
        int64_t times = sender->get_times();
        m_nodeManager->onTick(times);
    }
private:

private:
    boost::intrusive_ptr<const AppConfig> m_config;
    boost::intrusive_ptr<AppInfoImpl> m_appInfo;
    boost::intrusive_ptr<NodeManager> m_nodeManager;
    xul::timer_holder m_timer;
};


class AppConfigImpl : public xul::root_options_proxy<AppConfig>
{
public:
    AppConfigImpl()
    {
        xul::options_wrapper opts(get_options());
        opts.add("httpPort", &httpPort, 18080);
        opts.add("tcpPort", &tcpPort, 18333);
        opts.add("maxNodeCount", &maxNodeCount, 30);
        opts.add("connectInterval", &connectInterval, 30);
        opts.add("dataDir", &dataDir, "");
        opts.add_binary_byte_count("dbCache", &dbCache, 450, "MB");
        opts.add("directNode", &directNode, "");
        opts.add("testNet", &testNet, false);
        // minimumChainWork = uint256::parse("000000000000000000000000000000000000000000f91c579d57cad4bc5278cc");
        minimumChainWork = uint256::parse("00000000000000000000000000000000000000000000000000000000000000cc");
    }

};

AppConfig* createAppConfig()
{
    return new AppConfigImpl;
}

BitCoinApp* createBitCoinApp()
{
    return new BitCoinAppImpl;
}

std::ostream& operator<<(std::ostream& os, const PeerAddress& addr)
{
    return os << xul::make_inet4_address(addr.ip) << ':' << addr.port;
}

xul::data_output_stream& operator<<(xul::data_output_stream& os, const PeerAddress& addr)
{
    return os << addr.ip << addr.port;
}

xul::data_input_stream& operator>>(xul::data_input_stream& is, PeerAddress& addr)
{
    return is >> addr.ip >> addr.port;
}

xul::inet_socket_address toSocketAddress(const PeerAddress& addr)
{
    return xul::inet_socket_address(addr.ip, addr.port);
}

static std::string formatVersion(int nVersion)
{
    if (nVersion % 100 == 0)
        return xul::strings::format("%d.%d.%d", nVersion / 1000000, (nVersion / 10000) % 100, (nVersion / 100) % 100);
    else
        return xul::strings::format("%d.%d.%d.%d", nVersion / 1000000, (nVersion / 10000) % 100, (nVersion / 100) % 100, nVersion % 100);
}

std::string formatUserAgent(const std::string& name, int nClientVersion, const std::vector<std::string>& comments)
{
    std::ostringstream ss;
    ss << "/";
    ss << name << ":" << formatVersion(nClientVersion);
    if (!comments.empty())
    {
        std::vector<std::string>::const_iterator it(comments.begin());
        ss << "(" << *it;
        for(++it; it != comments.end(); ++it)
            ss << "; " << *it;
        ss << ")";
    }
    ss << "/";
    return ss.str();
}

}
