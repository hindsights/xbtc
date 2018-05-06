#pragma once

#include "util/number.hpp"
#include <vector>
#include <stdint.h>

namespace xbtc {


class Block;

class MerkleTree
{
public:
    static uint256 hashSiblingLeaves(const uint256& x, const uint256& y);
    static uint256 build(const Block* block, bool* mutated = nullptr);
    static void compute(const std::vector<uint256>& leaves, uint256* proot, bool* pmutated, uint32_t branchpos, std::vector<uint256>* pbranch);
    static uint256 computeRoot(const std::vector<uint256>& leaves, bool* mutated = nullptr);
};

}
