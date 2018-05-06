#pragma once

#include "data/Block.hpp"
#include <xul/lang/object_ptr.hpp>


namespace xbtc {


class ChainParams : public xul::object
{
public:
    boost::intrusive_ptr<Block> genesisBlock;
    boost::intrusive_ptr<BlockIndex> genesisBlockIndex;

    uint32_t blockTag;
};


}
