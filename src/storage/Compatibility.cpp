#include "Compatibility.hpp"
#include "data/Block.hpp"
#include "data/Transaction.hpp"


namespace xbtc {


static const uint256 BLOCK_HASH_91842 = uint256::parse("00000000000a4d0a398161ffc163c503763b1f4360639393e0e4c8e300e0caec");
static const uint256 BLOCK_HASH_91880 = uint256::parse("00000000000743f190a18c5577a3c2d2a1f610ae9601ac046a38084ccb7cd721");

bool Compatibility::forbidDuplicateTransaction(const BlockIndex* block)
{
    return (block->height != 91842 || block->getHash() != BLOCK_HASH_91842)
        && (block->height != 91880 || block->getHash() != BLOCK_HASH_91880);
}


}
