#pragma once

#include "data/Block.hpp"
#include <xul/lang/object_ptr.hpp>


namespace xbtc {


class ChainParams : public xul::object
{
public:
    boost::intrusive_ptr<Block> genesisBlock;
    std::vector<std::string> dnsSeeds;

    uint32_t protocolMagic;
    int defaultPort;
};


ChainParams* createMainChainParams();
ChainParams* createTestNetChainParams();


}
