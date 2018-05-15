#include "Script.hpp"
#include "util/serialization.hpp"
#include <xul/util/test_case.hpp>

namespace xbtc {


const char* getOpName(opcodetype opcode)
{
    switch (opcode)
    {
    // push value
    case OP_0                      : return "0";
    case OP_PUSHDATA1              : return "OP_PUSHDATA1";
    case OP_PUSHDATA2              : return "OP_PUSHDATA2";
    case OP_PUSHDATA4              : return "OP_PUSHDATA4";
    case OP_1NEGATE                : return "-1";
    case OP_RESERVED               : return "OP_RESERVED";
    case OP_1                      : return "1";
    case OP_2                      : return "2";
    case OP_3                      : return "3";
    case OP_4                      : return "4";
    case OP_5                      : return "5";
    case OP_6                      : return "6";
    case OP_7                      : return "7";
    case OP_8                      : return "8";
    case OP_9                      : return "9";
    case OP_10                     : return "10";
    case OP_11                     : return "11";
    case OP_12                     : return "12";
    case OP_13                     : return "13";
    case OP_14                     : return "14";
    case OP_15                     : return "15";
    case OP_16                     : return "16";

    // control
    case OP_NOP                    : return "OP_NOP";
    case OP_VER                    : return "OP_VER";
    case OP_IF                     : return "OP_IF";
    case OP_NOTIF                  : return "OP_NOTIF";
    case OP_VERIF                  : return "OP_VERIF";
    case OP_VERNOTIF               : return "OP_VERNOTIF";
    case OP_ELSE                   : return "OP_ELSE";
    case OP_ENDIF                  : return "OP_ENDIF";
    case OP_VERIFY                 : return "OP_VERIFY";
    case OP_RETURN                 : return "OP_RETURN";

    // stack ops
    case OP_TOALTSTACK             : return "OP_TOALTSTACK";
    case OP_FROMALTSTACK           : return "OP_FROMALTSTACK";
    case OP_2DROP                  : return "OP_2DROP";
    case OP_2DUP                   : return "OP_2DUP";
    case OP_3DUP                   : return "OP_3DUP";
    case OP_2OVER                  : return "OP_2OVER";
    case OP_2ROT                   : return "OP_2ROT";
    case OP_2SWAP                  : return "OP_2SWAP";
    case OP_IFDUP                  : return "OP_IFDUP";
    case OP_DEPTH                  : return "OP_DEPTH";
    case OP_DROP                   : return "OP_DROP";
    case OP_DUP                    : return "OP_DUP";
    case OP_NIP                    : return "OP_NIP";
    case OP_OVER                   : return "OP_OVER";
    case OP_PICK                   : return "OP_PICK";
    case OP_ROLL                   : return "OP_ROLL";
    case OP_ROT                    : return "OP_ROT";
    case OP_SWAP                   : return "OP_SWAP";
    case OP_TUCK                   : return "OP_TUCK";

    // splice ops
    case OP_CAT                    : return "OP_CAT";
    case OP_SUBSTR                 : return "OP_SUBSTR";
    case OP_LEFT                   : return "OP_LEFT";
    case OP_RIGHT                  : return "OP_RIGHT";
    case OP_SIZE                   : return "OP_SIZE";

    // bit logic
    case OP_INVERT                 : return "OP_INVERT";
    case OP_AND                    : return "OP_AND";
    case OP_OR                     : return "OP_OR";
    case OP_XOR                    : return "OP_XOR";
    case OP_EQUAL                  : return "OP_EQUAL";
    case OP_EQUALVERIFY            : return "OP_EQUALVERIFY";
    case OP_RESERVED1              : return "OP_RESERVED1";
    case OP_RESERVED2              : return "OP_RESERVED2";

    // numeric
    case OP_1ADD                   : return "OP_1ADD";
    case OP_1SUB                   : return "OP_1SUB";
    case OP_2MUL                   : return "OP_2MUL";
    case OP_2DIV                   : return "OP_2DIV";
    case OP_NEGATE                 : return "OP_NEGATE";
    case OP_ABS                    : return "OP_ABS";
    case OP_NOT                    : return "OP_NOT";
    case OP_0NOTEQUAL              : return "OP_0NOTEQUAL";
    case OP_ADD                    : return "OP_ADD";
    case OP_SUB                    : return "OP_SUB";
    case OP_MUL                    : return "OP_MUL";
    case OP_DIV                    : return "OP_DIV";
    case OP_MOD                    : return "OP_MOD";
    case OP_LSHIFT                 : return "OP_LSHIFT";
    case OP_RSHIFT                 : return "OP_RSHIFT";
    case OP_BOOLAND                : return "OP_BOOLAND";
    case OP_BOOLOR                 : return "OP_BOOLOR";
    case OP_NUMEQUAL               : return "OP_NUMEQUAL";
    case OP_NUMEQUALVERIFY         : return "OP_NUMEQUALVERIFY";
    case OP_NUMNOTEQUAL            : return "OP_NUMNOTEQUAL";
    case OP_LESSTHAN               : return "OP_LESSTHAN";
    case OP_GREATERTHAN            : return "OP_GREATERTHAN";
    case OP_LESSTHANOREQUAL        : return "OP_LESSTHANOREQUAL";
    case OP_GREATERTHANOREQUAL     : return "OP_GREATERTHANOREQUAL";
    case OP_MIN                    : return "OP_MIN";
    case OP_MAX                    : return "OP_MAX";
    case OP_WITHIN                 : return "OP_WITHIN";

    // crypto
    case OP_RIPEMD160              : return "OP_RIPEMD160";
    case OP_SHA1                   : return "OP_SHA1";
    case OP_SHA256                 : return "OP_SHA256";
    case OP_HASH160                : return "OP_HASH160";
    case OP_HASH256                : return "OP_HASH256";
    case OP_CODESEPARATOR          : return "OP_CODESEPARATOR";
    case OP_CHECKSIG               : return "OP_CHECKSIG";
    case OP_CHECKSIGVERIFY         : return "OP_CHECKSIGVERIFY";
    case OP_CHECKMULTISIG          : return "OP_CHECKMULTISIG";
    case OP_CHECKMULTISIGVERIFY    : return "OP_CHECKMULTISIGVERIFY";

    // expansion
    case OP_NOP1                   : return "OP_NOP1";
    case OP_CHECKLOCKTIMEVERIFY    : return "OP_CHECKLOCKTIMEVERIFY";
    case OP_CHECKSEQUENCEVERIFY    : return "OP_CHECKSEQUENCEVERIFY";
    case OP_NOP4                   : return "OP_NOP4";
    case OP_NOP5                   : return "OP_NOP5";
    case OP_NOP6                   : return "OP_NOP6";
    case OP_NOP7                   : return "OP_NOP7";
    case OP_NOP8                   : return "OP_NOP8";
    case OP_NOP9                   : return "OP_NOP9";
    case OP_NOP10                  : return "OP_NOP10";

    case OP_INVALIDOPCODE          : return "OP_INVALIDOPCODE";

    // Note:
    //  The template matching params OP_SMALLINTEGER/etc are defined in opcodetype enum
    //  as kind of implementation hack, they are *NOT* real opcodes.  If found in real
    //  Script, just let the default: case deal with them.

    default:
        return "OP_UNKNOWN";
    }
}

const char* ScriptErrorString(const ScriptError serror)
{
    switch (serror)
    {
        case SCRIPT_ERR_OK:
            return "No error";
        case SCRIPT_ERR_EVAL_FALSE:
            return "Script evaluated without error but finished with a false/empty top stack element";
        case SCRIPT_ERR_VERIFY:
            return "Script failed an OP_VERIFY operation";
        case SCRIPT_ERR_EQUALVERIFY:
            return "Script failed an OP_EQUALVERIFY operation";
        case SCRIPT_ERR_CHECKMULTISIGVERIFY:
            return "Script failed an OP_CHECKMULTISIGVERIFY operation";
        case SCRIPT_ERR_CHECKSIGVERIFY:
            return "Script failed an OP_CHECKSIGVERIFY operation";
        case SCRIPT_ERR_NUMEQUALVERIFY:
            return "Script failed an OP_NUMEQUALVERIFY operation";
        case SCRIPT_ERR_SCRIPT_SIZE:
            return "Script is too big";
        case SCRIPT_ERR_PUSH_SIZE:
            return "Push value size limit exceeded";
        case SCRIPT_ERR_OP_COUNT:
            return "Operation limit exceeded";
        case SCRIPT_ERR_STACK_SIZE:
            return "Stack size limit exceeded";
        case SCRIPT_ERR_SIG_COUNT:
            return "Signature count negative or greater than pubkey count";
        case SCRIPT_ERR_PUBKEY_COUNT:
            return "Pubkey count negative or limit exceeded";
        case SCRIPT_ERR_BAD_OPCODE:
            return "Opcode missing or not understood";
        case SCRIPT_ERR_DISABLED_OPCODE:
            return "Attempted to use a disabled opcode";
        case SCRIPT_ERR_INVALID_STACK_OPERATION:
            return "Operation not valid with the current stack size";
        case SCRIPT_ERR_INVALID_ALTSTACK_OPERATION:
            return "Operation not valid with the current altstack size";
        case SCRIPT_ERR_OP_RETURN:
            return "OP_RETURN was encountered";
        case SCRIPT_ERR_UNBALANCED_CONDITIONAL:
            return "Invalid OP_IF construction";
        case SCRIPT_ERR_NEGATIVE_LOCKTIME:
            return "Negative locktime";
        case SCRIPT_ERR_UNSATISFIED_LOCKTIME:
            return "Locktime requirement not satisfied";
        case SCRIPT_ERR_SIG_HASHTYPE:
            return "Signature hash type missing or not understood";
        case SCRIPT_ERR_SIG_DER:
            return "Non-canonical DER signature";
        case SCRIPT_ERR_MINIMALDATA:
            return "Data push larger than necessary";
        case SCRIPT_ERR_SIG_PUSHONLY:
            return "Only non-push operators allowed in signatures";
        case SCRIPT_ERR_SIG_HIGH_S:
            return "Non-canonical signature: S value is unnecessarily high";
        case SCRIPT_ERR_SIG_NULLDUMMY:
            return "Dummy CHECKMULTISIG argument must be zero";
        case SCRIPT_ERR_MINIMALIF:
            return "OP_IF/NOTIF argument must be minimal";
        case SCRIPT_ERR_SIG_NULLFAIL:
            return "Signature must be zero for failed CHECK(MULTI)SIG operation";
        case SCRIPT_ERR_DISCOURAGE_UPGRADABLE_NOPS:
            return "NOPx reserved for soft-fork upgrades";
        case SCRIPT_ERR_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM:
            return "Witness version reserved for soft-fork upgrades";
        case SCRIPT_ERR_PUBKEYTYPE:
            return "Public key is neither compressed or uncompressed";
        case SCRIPT_ERR_CLEANSTACK:
            return "Extra items left on stack after execution";
        case SCRIPT_ERR_WITNESS_PROGRAM_WRONG_LENGTH:
            return "Witness program has incorrect length";
        case SCRIPT_ERR_WITNESS_PROGRAM_WITNESS_EMPTY:
            return "Witness program was passed an empty witness";
        case SCRIPT_ERR_WITNESS_PROGRAM_MISMATCH:
            return "Witness program hash mismatch";
        case SCRIPT_ERR_WITNESS_MALLEATED:
            return "Witness requires empty scriptSig";
        case SCRIPT_ERR_WITNESS_MALLEATED_P2SH:
            return "Witness requires only-redeemscript scriptSig";
        case SCRIPT_ERR_WITNESS_UNEXPECTED:
            return "Witness provided for non-witness script";
        case SCRIPT_ERR_WITNESS_PUBKEYTYPE:
            return "Using non-compressed keys in segwit";
        case SCRIPT_ERR_UNKNOWN_ERROR:
        case SCRIPT_ERR_ERROR_COUNT:
        default: break;
    }
    return "unknown error";
}

const signed char p_util_hexdigit[256] =
{ -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,
  -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, };

signed char HexDigit(char c)
{
    return p_util_hexdigit[(unsigned char)c];
}


std::string ScriptUtils::encodeScriptNumber(int64_t val)
{
    std::string s;
    if (val == 0)
        return s;

    const bool neg = val < 0;
    uint64_t absvalue = neg ? -val : val;
    while (absvalue)
    {
        uint8_t bytevalue = static_cast<uint8_t>(absvalue & 0xff);
        absvalue >>= 8;
        s.push_back(bytevalue);
    }
    uint8_t lastByte = static_cast<uint8_t>(s.back());
    if (lastByte & 0x80)
        s.push_back(neg ? 0x80 : 0);
    else if (neg)
        s.back() = (lastByte | 0x80);
    return s;
}

bool ScriptUtils::parseHex(std::string& out, const char* psz)
{
    out.clear();
    // convert hex dump to vector
    while (true)
    {
        while (isspace(*psz))
            psz++;
        signed char c = HexDigit(*psz++);
        if (c == (signed char)-1)
            break;
        unsigned char n = (c << 4);
        c = HexDigit(*psz++);
        if (c == (signed char)-1)
            break;
        n |= c;
        out.push_back(n);
    }
    return true;
}

bool ScriptUtils::decodeScriptNumber(int64_t& val, const std::string& s)
{
    if (s.empty())
    {
        val = 0;
        return true;
    }

    uint64_t tempval = 0;
    for (int i = 0; i < s.size(); ++i)
    {
        uint64_t byteval = static_cast<uint8_t>(s[i]);
        assert(byteval >= 0);
        if (byteval > 0)
            tempval |= (byteval << (8*i));
    }

    // If the input vector's most significant byte is 0x80, remove it from
    // the result's msb and return a negative.
    uint8_t lastByte = s.back();
    if (lastByte & 0x80)
        val = -((int64_t)(tempval & ~(0x80ULL << (8 * (s.size() - 1)))));
    else
    {
        assert(tempval <= INT64_MAX);
        val = static_cast<int64_t>(tempval);
    }

    return true;
}

class ScriptBuilder
{
public:
    std::ostringstream oss;
    xul::ostream_data_output_stream os;

    ScriptBuilder();
    void pushByte(uint8_t val);
    void pushOpCode(opcodetype opcode);
    void pushInteger(int64_t val);
    void pushScriptNumber(int64_t val);
    void pushVector(const std::vector<uint8_t>& data);
    void pushString(const std::string& data);
    void pushBytes(const uint8_t* data, int size);
    void pushHexString(const std::string& s);
    std::string getString() const { return oss.str(); }
};


ScriptBuilder::ScriptBuilder() : os(oss, false)
{
}

void ScriptBuilder::pushInteger(int64_t val)
{
    if (val == -1 || (val >= 1 && val <= 16))
    {
        os.write_byte(val + (OP_1 - 1));
    }
    else if (val == 0)
    {
        os.write_byte(OP_0);
    }
    else
    {
        pushScriptNumber(val);
    }
}

void ScriptBuilder::pushScriptNumber(int64_t val)
{
    std::string s = ScriptUtils::encodeScriptNumber(val);
    VarEncoding::writeString(os, s);
}

void ScriptBuilder::pushByte(uint8_t val)
{
    os.write_byte(val);
}

void ScriptBuilder::pushOpCode(opcodetype opcode)
{
    os.write_byte(opcode);
}

void ScriptBuilder::pushVector(const std::vector<uint8_t>& data)
{
    pushBytes(data.data(), data.size());
}

void ScriptBuilder::pushString(const std::string& data)
{
    pushBytes(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

void ScriptBuilder::pushBytes(const uint8_t* data, int size)
{
    if (size < OP_PUSHDATA1)
    {
        os.write_byte(size);
    }
    else if (size <= 0xff)
    {
        os.write_byte(OP_PUSHDATA1);
        os.write_byte(size);
    }
    else if (size <= 0xffff)
    {
        os.write_byte(OP_PUSHDATA2);
        os.write_uint16(size);
    }
    else
    {
        os.write_byte(OP_PUSHDATA4);
        os.write_uint32(size);
    }
    os.write_bytes(data, size);
}



xul::data_input_stream& operator>>(xul::data_input_stream& is, const ScriptNumberReader& reader)
{
    assert(false);
    return is;
}
xul::data_output_stream& operator<<(xul::data_output_stream& os, const ScriptIntegerWriter& writer)
{
    int64_t val = writer.value;
    if (val == -1 || (val >= 1 && val <= 16))
    {
        os.write_byte(val + (OP_1 - 1));
    }
    else if (val == 0)
    {
        os.write_byte(OP_0);
    }
    else
    {
        os << ScriptNumberWriter(val);
    }
    return os;
}
xul::data_output_stream& operator<<(xul::data_output_stream& os, const ScriptNumberWriter& writer)
{
    std::string s = ScriptUtils::encodeScriptNumber(writer.value);
    return os << ScriptStringWriter(s);
}

xul::data_input_stream& operator>>(xul::data_input_stream& is, const ScriptStringReader& reader)
{
    assert(false);
    return is;
}
xul::data_output_stream& operator<<(xul::data_output_stream& os, const ScriptStringWriter& writer)
{
    int size = writer.value.size();
    if (size < OP_PUSHDATA1)
    {
        os.write_byte(size);
    }
    else if (size <= 0xff)
    {
        os.write_byte(OP_PUSHDATA1);
        os.write_byte(size);
    }
    else if (size <= 0xffff)
    {
        os.write_byte(OP_PUSHDATA2);
        os.write_uint16(size);
    }
    else
    {
        os.write_byte(OP_PUSHDATA4);
        os.write_uint32(size);
    }
    if (size > 0)
    {
        os.write_bytes(reinterpret_cast<const uint8_t*>(writer.value.data()), size);
    }
    return os;
}


}

#ifdef XUL_RUN_TEST


namespace xbtc {

class ScriptNumberTestCase : public xul::test_case
{
public:
    virtual void run()
    {
        testDecode1();
        testDecode2();
    }
    void testDecode1()
    {
        int64_t val;
        assert(ScriptUtils::decodeScriptNumber(val, std::string("\x81", 1)) && val == -1);
        assert(ScriptUtils::decodeScriptNumber(val, std::string("\xe4", 1)) && val == -100);
        assert(ScriptUtils::decodeScriptNumber(val, std::string("\x64", 1)) && val == 100);
    }
    void testDecode2()
    {
        int64_t val;
        assert(ScriptUtils::decodeScriptNumber(val, std::string()) && val == 0);
        assert(ScriptUtils::decodeScriptNumber(val, std::string("\xFF\xFF\xFF\xFF", 4)) && val == -0x7fffffffULL);
        assert(ScriptUtils::decodeScriptNumber(val, std::string("\xFF\xFF\xFF\x7F", 4)) && val == 0x7fffffffULL);
    }
};

XUL_TEST_SUITE_REGISTRATION(ScriptNumberTestCase);

}

#endif
