#include "serialization.hpp"
#include "number.hpp"

#include <xul/data/big_number_io.hpp>
#include <xul/data/bit_converter.hpp>

#include <assert.h>


namespace xbtc {


xul::data_input_stream& operator>>(xul::data_input_stream& is, UInt48& val)
{
    uint32_t lsb = 0;
    uint16_t msb = 0;
    is >> lsb >> msb;
    if (is)
    {
        val.value = xul::bit_converter::make_qword(msb, lsb);
    }
    return is;
}

xul::data_output_stream& operator<<(xul::data_output_stream& os, const UInt48& val)
{
    uint32_t lsb = xul::bit_converter::low_dword(val.value);
    uint16_t msb = static_cast<uint16_t>(xul::bit_converter::high_dword(val.value));
    return os << lsb << msb;
}

bool VarEncoding::readString(xul::data_input_stream& is, std::string& s, int maxsize)
{
    uint64_t size = 0;
    if (!readCompactSize(is, size))
        return false;
    if (size > maxsize)
    {
        is.set_bad();
        return false;
    }
    return is.read_string(s, static_cast<int>(size));
}
void VarEncoding::writeString(xul::data_output_stream& os, const std::string& s)
{
    writeCompactSize(os, s.size());
    os.write_array(s.data(), s.size());
}
bool VarEncoding::readCompactSize(xul::data_input_stream& is, uint64_t& val)
{
    uint8_t tag;
    if (!is.read_byte(tag))
        return false;
    if (0xFD == tag)
    {
        uint16_t val16;
        is >> val16;
        val = val16;
        return is.good();
    }
    if (0xFE == tag)
    {
        uint32_t val32;
        is >> val32;
        val = val32;
        return is.good();
    }
    if (0xFF == tag)
    {
        is >> val;
        return is.good();
    }
    val = tag;
    assert(is.good());
    return true;
}
void VarEncoding::writeCompactSize(xul::data_output_stream& os, const uint64_t& val)
{
    if (val < 0xFD)
    {
        os.write_uint8(static_cast<uint8_t>(val));
        return;
    }
    if (val <= 0xFFFF)
    {
        os.write_uint8(0xFD);
        os.write_uint16(static_cast<uint16_t>(val));
        return;
    }
    if (val <= 0xFFFFFFFF)
    {
        os.write_uint8(0xFE);
        os.write_uint32(static_cast<uint32_t>(val));
        return;
    }
    os.write_uint8(0xFF);
    os.write_uint64(val);
}

xul::data_input_stream& operator>>(xul::data_input_stream& is, const LimitStringReader& reader)
{
    VarEncoding::readString(is, reader.value, reader.maxSize);
    return is;
}
xul::data_output_stream& operator<<(xul::data_output_stream& os, const LimitStringWriter& writer)
{
    VarEncoding::writeString(os, writer.value);
    return os;
}


}
