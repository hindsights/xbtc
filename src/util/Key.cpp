#include "Key.hpp"

#include <xul/log/log.hpp>
#include <xul/text/hex_encoding.hpp>

#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/crypto.h>


namespace xbtc {


PublicKey::PublicKey()
{
    m_key = EC_KEY_new_by_curve_name(NID_secp256k1);
    assert(m_key);
}

bool PublicKey::verify(const std::string& pubkey, const uint256& hash, const std::string& sig)
{
    PublicKey key;
    if (!key.assign(pubkey))
        return false;
    return key.verify(hash, sig);
}

bool PublicKey::assign(const std::string& keydata)
{
    const uint8_t* data = reinterpret_cast<const uint8_t*>(keydata.data());
    if (!o2i_ECPublicKey(&m_key, &data, keydata.size()))
        return false;
    return true;
}

int xbtc_ECDSA_verify(int type, const unsigned char *dgst, int dgst_len,
                 const unsigned char *sigbuf, int sig_len, EC_KEY *eckey)
{
    ECDSA_SIG *s;
    const unsigned char *p = sigbuf;
    unsigned char *der = NULL;
    int derlen = -1;
    int ret = -1;

    s = ECDSA_SIG_new();
    if (s == NULL)
        return (ret);
    if (d2i_ECDSA_SIG(&s, &p, sig_len) == NULL)
        goto err;
    /* Ensure signature uses DER and doesn't have trailing garbage */
    derlen = i2d_ECDSA_SIG(s, &der);
    if (derlen != sig_len || memcmp(sigbuf, der, derlen))
    {
        XUL_APP_WARN("xbtc_ECDSA_verify sig will garbage " << xul::make_tuple(derlen, sig_len)
            << " " << xul::hex_encoding::lower_case().encode(sigbuf, sig_len)
            << " " << xul::hex_encoding::lower_case().encode(der, derlen));
        // goto err;
    }
    ret = ECDSA_do_verify(dgst, dgst_len, s, eckey);
 err:
    if (derlen > 0) {
        OPENSSL_cleanse(der, derlen);
        OPENSSL_free(der);
    }
    ECDSA_SIG_free(s);
    return (ret);
}

bool PublicKey::verify(const uint256& hash, const std::string& sig)
{
    const uint8_t* sigdata = reinterpret_cast<const uint8_t*>(sig.data());
    int ret = xbtc_ECDSA_verify(0, hash.data(), hash.size(), sigdata, sig.size(), m_key);
//    int ret = ECDSA_verify(0, hash.data(), hash.size(), sigdata, sig.size(), m_key);
    if (ret != 1)
        return false;
    return true;
}


}
