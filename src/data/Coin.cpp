#include "data/Coin.hpp"
#include "data/Transaction.hpp"
#include "version.hpp"
#include "util/serialization.hpp"
#include "util/Hasher.hpp"

#include <xul/data/big_number_io.hpp>
#include <xul/lang/object_impl.hpp>
#include <xul/macro/minmax.hpp>

namespace xbtc {


xul::data_input_stream& operator>>(xul::data_input_stream& is, Coin& coin)
{
    uint32_t val;
    is >> coin.output >> val;
    if (is.good())
    {
        coin.height = val >> 1;
        coin.isCoinBase = (val & 1);
    }
    return is;
}

xul::data_output_stream& operator<<(xul::data_output_stream& os, const Coin& coin)
{
    assert(coin.height >= 0);
    uint32_t val = static_cast<uint32_t>(coin.height);
    val <<= 2;
    if (coin.isCoinBase)
    {
        val++;
    }
    return os << coin.output << val;
}


}
