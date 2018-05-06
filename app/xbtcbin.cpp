#include "app.hpp"
#include "AppConfig.hpp"

#include <xul/lang/object_ptr.hpp>
#include <xul/os/file_system.hpp>
#include <xul/util/simple_program_options.hpp>
#include <stdio.h>


int main(int argc, char** argv)
{
    printf("hello, xbitcoin binary %s\n", argv[0]);
    xul::simple_program_options opts;
    opts.load(argc - 1, argv + 1);
    std::string confiefile = opts.get_option("--conf", "xbtc.conf");
    boost::intrusive_ptr<xbtc::BitCoinApp> app(xbtc::createBitCoinApp());
    boost::intrusive_ptr<xbtc::AppConfig> config = xbtc::createAppConfig();
    config->parse_file(confiefile.c_str());
    if (config->dataDir.empty() || !xul::file_system::ensure_directory_exists(config->dataDir.c_str()))
    {
        printf("invalid data dir: %s\n", config->dataDir.c_str());
        return 11;
    }
    app->start(config.get());
    app->wait();
    return 0;
}
