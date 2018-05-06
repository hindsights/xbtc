#pragma once

#include "util/number.hpp"
#include <xul/util/options.hpp>
#include <string>

namespace xbtc {


class AppConfig : public xul::root_options
{
public:
    int tcpPort;
    int httpPort;
    int connectInterval;
    int tcpPortRange;
    int maxNodeCount;
    std::string dataDir;
    int dbCache;
    uint256 minimumChainWork;
    std::string directNode;
};


AppConfig* createAppConfig();


}
