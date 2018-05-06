#pragma once

#include <xul/data/big_number.hpp>
#include <vector>
#include <stdint.h>

namespace xul {
class data_input_stream;
class data_output_stream;
}

namespace xbtc {


using xul::uint256;

class UInt48
{
public:
    uint64_t value;
    UInt48() : value(0) {}
};

xul::data_input_stream& operator>>(xul::data_input_stream& is, UInt48& val);
xul::data_output_stream& operator<<(xul::data_output_stream& os, const UInt48& val);


}
