#pragma once

#include <xul/lang/object.hpp>
#include <string>
#include <vector>

namespace xbtc {


class AppConfig;


class BitCoinApp : public xul::object
{
public:
    virtual bool start(const AppConfig* config) = 0;
    virtual void wait() = 0;
    virtual void stop() = 0;
};


BitCoinApp* createBitCoinApp();
std::string formatUserAgent(const std::string& name, int nClientVersion, const std::vector<std::string>& comments);

}
