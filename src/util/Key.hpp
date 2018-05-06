#pragma once

#include "util/number.hpp"
#include <string>
#include <stdint.h>

struct ec_key_st;
typedef struct ec_key_st EC_KEY;

namespace xul {
class data_input_stream;
class data_output_stream;
}

namespace xbtc {


class PublicKey
{
public:
    static bool verify(const std::string& pubkey, const uint256& hash, const std::string& sig);

    PublicKey();
    bool assign(const std::string& keydata);
    bool verify(const uint256& hash, const std::string& sig);

private:
    EC_KEY* m_key;
};

}
