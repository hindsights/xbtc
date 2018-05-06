#pragma once

#include "number.hpp"
#include <xul/crypto/openssl/openssl_hasher.hpp>
#include <xul/crypto/hashing.hpp>
#include <xul/log/log.hpp>
#include <xul/data/big_number_io.hpp>


namespace xbtc {


class Hasher256 : public xul::hasher, public xul::hasher_mixin<Hasher256, uint256>
{
public:
    template <typename T>
    static uint256 hash_data(const T& obj)
    {
        return xul::hashing::hash_data<Hasher256, T>(false, obj);
    }
    template <typename ...ARGS>
    static uint256 hash_data(const ARGS&... args)
    {
        return xul::hashing::hash_data<Hasher256>(false, args...);
    }

public:
    typedef xul::openssl_sha256_hasher::digest_type digest_type;
    static const int digest_length = xul::openssl_sha256_hasher::digest_length;

    Hasher256()
    {
    }
    virtual int get_max_digest_length() const
    {
        return digest_length;
    }
    virtual void reset()
    {
        m_hasher.reset();
    }
    virtual void update(const uint8_t* data, int size)
    {
        m_hasher.update(data, size);
    }
    void update(const char* data, int size)
    {
        m_hasher.update(reinterpret_cast<const uint8_t*>(data), size);
    }
    virtual int finalize(uint8_t* digest, int size)
    {
        if (size < digest_length)
            return 0;
        uint256 hash = m_hasher.finalize();
        xul::openssl_sha256_hasher hasher;
        hasher.update(hash.data(), hash.size());
        return hasher.finalize(digest, size);
    }
    uint256 finalize()
    {
        uint256 digest = m_hasher.finalize();
        return xul::openssl_sha256_hasher::hash(&digest[0], 32);
    }

private:
    xul::openssl_sha256_hasher m_hasher;
};

typedef Hasher256 Hasher;


class Hasher160 : public xul::hasher, public xul::hasher_mixin<Hasher160, xul::uint160>
{
public:
    typedef xul::uint160 digest_type;
    static const int digest_length = xul::uint160::byte_size;

    Hasher160()
    {
    }
    virtual int get_max_digest_length() const
    {
        return digest_length;
    }
    virtual void reset()
    {
        m_hasher.reset();
    }
    virtual void update(const uint8_t* data, int size)
    {
        m_hasher.update(data, size);
    }
    void update(const char* data, int size)
    {
        m_hasher.update(reinterpret_cast<const uint8_t*>(data), size);
    }
    virtual int finalize(uint8_t* digest, int size)
    {
        if (size < digest_length)
            return 0;
        uint256 hash = m_hasher.finalize();
        xul::openssl_ripemd160_hasher hasher;
        hasher.update(hash.data(), hash.size());
        return hasher.finalize(digest, size);
    }
    xul::uint160 finalize()
    {
        uint256 digest = m_hasher.finalize();
        return xul::openssl_ripemd160_hasher::hash(&digest[0], 32);
    }

private:
    xul::openssl_sha256_hasher m_hasher;
};


class SipHasher
{
public:
    typedef uint64_t digest_type;

    static uint64_t hashUInt256(uint64_t k0, uint64_t k1, const uint256& val);
    static uint64_t hashUInt256WithExtra(uint64_t k0, uint64_t k1, const uint256& val, uint32_t extra);

    SipHasher(uint64_t k0, uint64_t k1);
    void update(uint64_t data);
    void update(const uint8_t* data, int size);
    void update(const char* data, int size)
    {
        update(reinterpret_cast<const uint8_t*>(data), size);
    }
    uint64_t finalize();

private:
    uint64_t v[4];
    uint64_t tmp;
    int count;
};


class RandomSipHasher
{
public:
    typedef uint64_t digest_type;

    RandomSipHasher();

    uint64_t hashUInt256(const uint256& val)
    {
        return SipHasher::hashUInt256(m_key0, m_key1, val);
    }
    uint64_t hashUInt256WithExtra(const uint256& val, uint32_t extra)
    {
        return SipHasher::hashUInt256WithExtra(m_key0, m_key1, val, extra);
    }

private:
    uint64_t m_key0;
    uint64_t m_key1;
};


}
