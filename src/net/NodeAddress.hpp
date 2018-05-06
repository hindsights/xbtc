#pragma once

#include <xul/net/inet4_address.hpp>
#include <xul/std/strings.hpp>
#include <xul/data/numerics.hpp>
#include <iosfwd>
#include <stdint.h>

namespace xul {
    class inet_socket_address;
    class data_input_stream;
    class data_output_stream;
}


namespace xbtc {


class PeerAddress
{
public:
    uint32_t ip;
    uint16_t port;

    explicit PeerAddress(uint32_t theIP, uint16_t thePort)
        : ip(theIP), port(thePort)
    {
    }
    PeerAddress() : ip(0), port(0)
    {
    }

    bool isValid() const
    {
        return ip > 0 && port > 0;
    }
    bool parse(const std::string& s)
    {
        auto parts = xul::strings::split_pair(s, ':');
        if (parts.first.empty() || parts.second.empty())
            return false;
        ip = xul::make_inet4_address(parts.first.c_str()).get_address();
        port = xul::numerics::parse(parts.second, 0);
        return isValid();
    }
};


class NodeAddress
{
public:
    PeerAddress address;
    uint64_t services;

    NodeAddress() : services(0) { }
    bool isValid() const
    {
        return address.isValid();
    }
};


inline bool operator<(const PeerAddress& x, const PeerAddress& y)
{
    if (x.ip != y.ip)
        return x.ip < y.ip;
    return x.port < y.port;
}

inline bool operator==(const PeerAddress& x, const PeerAddress& y)
{
    return x.ip == y.ip && x.port == y.port;
}

inline bool operator!=(const PeerAddress& x, const PeerAddress& y)
{
    return !(x == y);
}


std::ostream& operator<<(std::ostream& os, const PeerAddress& addr);
xul::data_output_stream& operator<<(xul::data_output_stream& os, const PeerAddress& addr);
xul::data_input_stream& operator>>(xul::data_input_stream& is, PeerAddress& addr);
xul::inet_socket_address toSocketAddress(const PeerAddress& addr);

std::ostream& operator<<(std::ostream& os, const NodeAddress& addr);
xul::data_output_stream& operator<<(xul::data_output_stream& os, const NodeAddress& addr);
xul::data_input_stream& operator>>(xul::data_input_stream& is, NodeAddress& addr);
xul::inet_socket_address toSocketAddress(const NodeAddress& addr);

}
