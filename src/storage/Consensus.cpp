#include "Consensus.hpp"
#include "data/Block.hpp"
#include "util/arith_uint256.hpp"

namespace xbtc {


uint256 Consensus::decodeCompactUInt256(uint32_t compact, bool* negative, bool* overflow)
{
    uint256 ret;
    int size = compact >> 24;
    uint32_t word = compact & 0x007fffff;
    if (size <= 3)
    {
        word >>= 8 * (3 - size);
        ret.assign(word);
    }
    else
    {
        ret.assign(word);
        ret <<= 8 * (size - 3);
    }
    if (negative)
        *negative = word != 0 && (compact & 0x00800000) != 0;
    if (overflow)
        *overflow = word != 0 && ((size > 34) ||
                                     (word > 0xff && size > 33) ||
                                     (word > 0xffff && size > 32));
    return ret;
}
bool Consensus::decodeCompactNumber(uint256& val, uint32_t compact)
{
    bool negative;
    bool overflow;
    uint256 target = decodeCompactUInt256(compact, &negative, &overflow);
    if (negative || overflow || target.is_null())
        return false;
    val = target;
    return true;
}

#if 0
uint256 Consensus::calcBlockProof(uint32_t bits)
{
    uint256 target;
    if (!decodeCompactNumber(target, bits))
        return uint256();
    // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
    // as it's too large for an arith_uint256. However, as 2**256 is at least as large
    // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
    // or ~bnTarget / (bnTarget+1) + 1.
    uint256 x = ~target;
    uint256 y = target + 1;
    uint256 z = x / y;
    return z + 1;
//    return (~target / (target + 1)) + 1;
}
#else

arith_uint256 GetBlockProof(uint32_t bits)
{
    arith_uint256 bnTarget;
    bool fNegative;
    bool fOverflow;
    bnTarget.set_compact(bits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0)
        return 0;
    // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
    // as it's too large for an arith_uint256. However, as 2**256 is at least as large
    // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
    // or ~bnTarget / (bnTarget+1) + 1.
    return (~bnTarget / (bnTarget + 1)) + 1;
}
uint256 Consensus::calcBlockProof(uint32_t bits)
{
    return GetBlockProof(bits).to_uint256();
}
#endif

}
