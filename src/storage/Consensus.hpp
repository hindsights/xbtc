#pragma once

#include "util/number.hpp"
#include <stdint.h>

namespace xbtc {


class Block;
class BlockIndex;
class Transaction;

class Consensus
{
public:
    static uint256 decodeCompactUInt256(uint32_t compact, bool* negative = nullptr, bool* overflow = nullptr);
    static bool decodeCompactNumber(uint256& val, uint32_t compact);
    static uint256 calcBlockProof(uint32_t bits);
};


}
