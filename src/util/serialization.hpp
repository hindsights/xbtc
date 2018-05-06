#pragma once

#include "number.hpp"
#include <xul/io/data_input_stream.hpp>
#include <xul/io/data_output_stream.hpp>
#include <xul/data/big_number_io.hpp>
#include <vector>
#include <stdint.h>

namespace xul {
class data_input_stream;
class data_output_stream;
}

namespace xbtc {


template <typename T>
class VarIntReader
{
public:
    T& value;
    explicit VarIntReader(T& val) : value(val) {}
};

template <typename T>
class VarIntWriter
{
public:
    const T& value;
    explicit VarIntWriter(const T& val) : value(val) {}
};

class LimitStringReader
{
public:
    std::string& value;
    int maxSize;
    explicit LimitStringReader(std::string& val, int maxsize) : value(val), maxSize(maxsize) {}
};

class LimitStringWriter
{
public:
    const std::string& value;
    explicit LimitStringWriter(const std::string& val) : value(val) {}
};

template <typename T, typename A>
class VarVectorReader
{
public:
    std::vector<T, A>& value;
    int maxCount;
    explicit VarVectorReader(std::vector<T, A>& val, int maxcount) : value(val), maxCount(maxcount) {}
};

template <typename T, typename A>
class VarVectorWriter
{
public:
    const std::vector<T, A>& value;
    explicit VarVectorWriter(const std::vector<T, A>& val) : value(val) {}
};

template <typename T, typename A>
inline VarVectorReader<T, A> makeVarReader(std::vector<T, A>& val, int maxcount)
{
    return VarVectorReader<T, A>(val, maxcount);
}

template <typename T, typename A>
inline VarVectorWriter<T, A> makeVarWriter(const std::vector<T, A>& val)
{
    return VarVectorWriter<T, A>(val);
}


template <typename T>
inline VarIntReader<T> makeVarReader(T& val)
{
    return VarIntReader<T>(val);
}

template <typename T>
inline VarIntWriter<T> makeVarWriter(const T& val)
{
    return VarIntWriter<T>(val);
}


class VarEncoding
{
public:
    static bool readString(xul::data_input_stream& is, std::string& s, int maxsize);
    static void writeString(xul::data_output_stream& os, const std::string& s);
    static bool readCompactSize(xul::data_input_stream& is, uint64_t& val);
    static void writeCompactSize(xul::data_output_stream& os, const uint64_t& val);

    template <typename T>
    static bool readInteger(xul::data_input_stream& is, T& val)
    {
        T n = 0;
        for (;;)
        {
            if (n > (std::numeric_limits<T>::max() >> 7))
                break;
            uint8_t byteval = 0;
            if (!is.read_byte(byteval))
                break;
            uint8_t byteval7 = byteval & 0x7F;
            n = (n << 7) | byteval7;
            if (byteval & 0x80)
            {
                if (n == std::numeric_limits<T>::max())
                    break;
                ++n;
            }
            else
            {
                val = n;
                return true;
            }
        }
        is.set_bad();
        return false;
    }
    template <typename T>
    static void writeInteger(xul::data_output_stream& os, const T& n)
    {
        const size_t tempbufsize = (sizeof(T) * 8 + 6) / 7;
        uint8_t tempbuf[tempbufsize];
        T val = n;
        int pos = tempbufsize - 1;
        bool ending = true;
        for (;;)
        {
            assert(pos >= 0);
            uint8_t byteval = val & 0x7F;
            tempbuf[pos] = ending ? byteval : (byteval | 0x80);
            if (val <= 0x7F)
                break;
            val = (val >> 7) - 1;
            --pos;
            ending = false;
        }
        os.write_bytes(tempbuf + pos, tempbufsize - pos);
    }

    template <typename T, typename A>
    static bool readVector(xul::data_input_stream& is, std::vector<T, A>& items, int maxcount)
    {
        uint64_t count = 0;
        if (!readCompactSize(is, count))
            return false;
        if (count > maxcount)
        {
            is.set_bad();
            return false;
        }
        for (int i = 0; i < static_cast<int>(count); ++i)
        {
            T item;
            is >> item;
            if (!is)
            {
                is.set_bad();
                return false;
            }
            items.push_back(item);
        }
        return true;
    }
    template <typename T, typename A>
    static void writeVector(xul::data_output_stream& os, const std::vector<T, A>& items)
    {
        writeCompactSize(os, items.size());
        for (const auto& item : items)
        {
            os << item;
        }
    }
};

xul::data_input_stream& operator>>(xul::data_input_stream& is, const LimitStringReader& reader);
xul::data_output_stream& operator<<(xul::data_output_stream& os, const LimitStringWriter& writer);

template <typename T, typename A>
inline xul::data_input_stream& operator>>(xul::data_input_stream& is, const VarVectorReader<T, A>& reader)
{
    VarEncoding::readVector(is, reader.value, reader.maxCount);
    return is;
}
template <typename T, typename A>
inline xul::data_output_stream& operator<<(xul::data_output_stream& os, const VarVectorWriter<T, A>& writer)
{
    VarEncoding::writeVector(os, writer.value);
    return os;
}

template <typename T>
inline xul::data_input_stream& operator>>(xul::data_input_stream& is, const VarIntReader<T>& reader)
{
    VarEncoding::readInteger(is, reader.value);
    return is;
}
template <typename T>
inline xul::data_output_stream& operator<<(xul::data_output_stream& os, const VarIntWriter<T>& writer)
{
    VarEncoding::writeInteger(os, writer.value);
    return os;
}


}


namespace xul {

template <typename T, typename A>
inline xul::data_output_stream& operator<<(xul::data_output_stream& os, const std::vector<T, A>& v)
{
    xbtc::VarEncoding::writeVector(os, v);
    return os;
}

inline xul::data_output_stream& operator<<(xul::data_output_stream& os, const std::string& s)
{
    xbtc::VarEncoding::writeString(os, s);
    return os;
}

}
