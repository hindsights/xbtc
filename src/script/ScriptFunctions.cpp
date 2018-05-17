#include "ScriptVM.hpp"
#include "ScriptFunction.hpp"
#include "Script.hpp"
#include "util/Hasher.hpp"

#include <xul/io/data_input_stream.hpp>
#include <xul/io/data_encoding.hpp>
#include <xul/crypto/hasher.hpp>
#include <xul/util/singleton.hpp>
#include <xul/std/strings.hpp>
#include <xul/log/log.hpp>


namespace xbtc {


const std::string vTrue(1, OP_TRUE);
const std::string vFalse;


namespace {


#define VM_CHECK(vm, cond, errcode) do { if (!(cond)) { vm->env->setError(errcode); assert(false); return; } } while (false)
#define VM_CHECK_PARAM(vm, cond) VM_CHECK(vm, cond, (ScriptError::SCRIPT_ERR_INVALID_PARAM))
#define VM_CHECK_PARAM_COUNT(vm, n) VM_CHECK_PARAM(vm, (vm->stack->size() >= n))

class Hashers : public xul::singleton<Hashers>
{
public:
    std::vector<std::shared_ptr<xul::hasher> > hashers;

    Hashers() {}

    void init()
    {
        hashers.resize(5);
        assert(OP_RIPEMD160 + 4 == OP_HASH256);
        hashers[0] = std::make_shared<xul::openssl_ripemd160_hasher>();
        hashers[OP_SHA1 - OP_RIPEMD160] = std::make_shared<xul::openssl_sha1_hasher>();
        hashers[OP_SHA256 - OP_RIPEMD160] = std::make_shared<xul::openssl_sha256_hasher>();
        hashers[OP_HASH160 - OP_RIPEMD160] = std::make_shared<Hasher160>();
        hashers[OP_HASH256 - OP_RIPEMD160] = std::make_shared<Hasher256>();
    }
};


int64_t scriptOp_1add(const int64_t& val) { return val + 1; }
int64_t scriptOp_1sub(const int64_t& val) { return val - 1; }
int64_t scriptOp_2mul(const int64_t& val) { return val << 1; }
int64_t scriptOp_2div(const int64_t& val) { return val >> 1; }
int64_t scriptOp_negate(const int64_t& val) { return -val; }
int64_t scriptOp_abs(const int64_t& val) { return val >= 0 ? val : -val; }
int64_t scriptOp_not(const int64_t& val) { return val == 0; }
int64_t scriptOp_0notEqual(const int64_t& val) { return val != 0; }

typedef int64_t (*ScriptUnaryNumberFunctionType)(const int64_t& val);
class UnaryNumberOps : public xul::singleton<UnaryNumberOps>
{
public:
    std::vector<ScriptUnaryNumberFunctionType> functions;

    UnaryNumberOps() {}

    void init()
    {
        functions.push_back(scriptOp_1add);
        functions.push_back(scriptOp_1sub);
        functions.push_back(scriptOp_2mul);
        functions.push_back(scriptOp_2div);
        functions.push_back(scriptOp_negate);
        functions.push_back(scriptOp_abs);
        functions.push_back(scriptOp_not);
        functions.push_back(scriptOp_0notEqual);
    }
};

int64_t scriptOp_add(const int64_t& x, const int64_t& y) { return x + y; }
int64_t scriptOp_sub(const int64_t& x, const int64_t& y) { return x - y; }
int64_t scriptOp_mul(const int64_t& x, const int64_t& y) { return x * y; }
int64_t scriptOp_div(const int64_t& x, const int64_t& y) { return x / y; }
int64_t scriptOp_mod(const int64_t& x, const int64_t& y) { return x % y; }
int64_t scriptOp_lshift(const int64_t& x, const int64_t& y) { return x << y; }
int64_t scriptOp_rshift(const int64_t& x, const int64_t& y) { return x >> y; }

int64_t scriptOp_boolAnd(const int64_t& x, const int64_t& y) { return x != 0 && y != 0; }
int64_t scriptOp_boolOr(const int64_t& x, const int64_t& y) { return x != 0 || y != 0; }
int64_t scriptOp_numberEqual(const int64_t& x, const int64_t& y) { return x == y; }
int64_t scriptOp_numberEqualVerify(const int64_t& x, const int64_t& y) { return x == y; }
int64_t scriptOp_numberNotEqual(const int64_t& x, const int64_t& y) { return x != y; }
int64_t scriptOp_lessThan(const int64_t& x, const int64_t& y) { return x < y; }
int64_t scriptOp_greaterThan(const int64_t& x, const int64_t& y) { return x > y; }
int64_t scriptOp_lessThanOrEqual(const int64_t& x, const int64_t& y) { return x <= y; }
int64_t scriptOp_greaterThanOrEqual(const int64_t& x, const int64_t& y) { return x >= y; }
int64_t scriptOp_min(const int64_t& x, const int64_t& y) { return x < y ? x : y; }
int64_t scriptOp_max(const int64_t& x, const int64_t& y) { return x > y ? x : y; }


typedef int64_t (*ScriptBinaryNumberFunctionType)(const int64_t& x, const int64_t& y);
class BinaryNumberOps : public xul::singleton<BinaryNumberOps>
{
public:
    std::vector<ScriptBinaryNumberFunctionType> functions;

    BinaryNumberOps() {}

    void init()
    {
        functions.push_back(scriptOp_add);
        functions.push_back(scriptOp_sub);
        functions.push_back(scriptOp_mul);
        functions.push_back(scriptOp_div);
        functions.push_back(scriptOp_mod);
        functions.push_back(scriptOp_lshift);
        functions.push_back(scriptOp_rshift);

        functions.push_back(scriptOp_boolAnd);
        functions.push_back(scriptOp_boolOr);
        functions.push_back(scriptOp_numberEqual);
        functions.push_back(scriptOp_numberEqualVerify);
        functions.push_back(scriptOp_numberNotEqual);
        functions.push_back(scriptOp_lessThan);
        functions.push_back(scriptOp_greaterThan);
        functions.push_back(scriptOp_lessThanOrEqual);
        functions.push_back(scriptOp_greaterThanOrEqual);
        functions.push_back(scriptOp_min);
        functions.push_back(scriptOp_max);
    }
};


void script_reserved(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    XUL_WARN(vm, "script_reserved " << xul::strings::format("opcode=0x%02X", opcode) << " " << getOpName(static_cast<opcodetype>(opcode)));
    assert(false);
}

void script_noop(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
}

void script_push0(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    assert(opcode < OP_PUSHDATA1);
    std::string s;
    if (opcode > 0 && !code.read_string(s, opcode))
    {
        vm->env->setError(1);
        return;
    }
    if (opcode == 0)
    {
        XUL_WARN(vm, "script_push0 empty data ");
    }
    vm->stack->push(s);
}

template <typename SizeT>
void script_pushdata(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    SizeT size = 0;
    code >> size;
    std::string s;
    if (code.read_string(s, size))
    {
        vm->stack->push(s);
    }
}
void script_push1(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    script_pushdata<uint8_t>(vm, opcode, code);
}
void script_push2(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    script_pushdata<uint16_t>(vm, opcode, code);
}
void script_push4(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    script_pushdata<uint32_t>(vm, opcode, code);
}

void script_integer(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    assert((opcode >= OP_1 && opcode <= OP_16) || opcode == OP_1NEGATE);
    int val = opcode - OP_1;
    std::string s = ScriptUtils::encodeScriptNumber(val + 1);
    vm->stack->push(s);
}

void script_equal(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    assert(opcode == OP_EQUAL || opcode == OP_EQUALVERIFY);
    VM_CHECK_PARAM_COUNT(vm, 2);
    const std::string& v1 = vm->stack->get(-1);
    const std::string& v2 = vm->stack->get(-2);
    bool ret = (v1 == v2);
    vm->stack->pop(2);
    vm->stack->pushBool(ret);
    if (opcode == OP_EQUALVERIFY)
    {
        vm->verify(SCRIPT_ERR_EQUALVERIFY);
    }
}

void script_numericUnaryOp(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    assert(opcode >= OP_1ADD && opcode <= OP_0NOTEQUAL);
    VM_CHECK_PARAM_COUNT(vm, 1);
    int opIndex = opcode - OP_1ADD;
    int64_t val;
    VM_CHECK_PARAM(vm, vm->getIntegerValue(-1, val));
    assert(opIndex >= 0 && opIndex < UnaryNumberOps::instance().functions.size());
    vm->stack->pop();
    vm->stack->pushInteger(UnaryNumberOps::instance().functions[opIndex](val));
}

void script_numericBinaryOp(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    assert(opcode >= OP_ADD && opcode <= OP_MAX);
    VM_CHECK_PARAM_COUNT(vm, 2);
    int opIndex = opcode - OP_ADD;
    int64_t x, y;
    VM_CHECK_PARAM(vm, vm->getIntegerValue(-2, x));
    VM_CHECK_PARAM(vm, vm->getIntegerValue(-1, y));
    assert(opIndex >= 0 && opIndex < BinaryNumberOps::instance().functions.size());
    vm->stack->pop(2);
    vm->stack->pushInteger(BinaryNumberOps::instance().functions[opIndex](x, y));
    if (opcode == OP_NUMEQUALVERIFY)
    {
        vm->verify(SCRIPT_ERR_NUMEQUALVERIFY);
    }
}

void script_within(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    // (x min max -- out)
    VM_CHECK_PARAM_COUNT(vm, 3);
    int64_t x, minval, maxval;
    VM_CHECK_PARAM(vm, vm->getIntegerValue(-3, x));
    VM_CHECK_PARAM(vm, vm->getIntegerValue(-2, minval));
    VM_CHECK_PARAM(vm, vm->getIntegerValue(-1, maxval));
    bool success = (minval <= x && x < maxval);
    vm->stack->pop(3);
    vm->stack->pushBool(success);
}

void script_dup(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    VM_CHECK_PARAM_COUNT(vm, 1);
    vm->stack->dup();
}
void script_dup2(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    VM_CHECK_PARAM_COUNT(vm, 2);
    vm->stack->dup(2);
}
void script_dup3(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    VM_CHECK_PARAM_COUNT(vm, 3);
    vm->stack->dup(3);
}

void script_drop(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    VM_CHECK_PARAM_COUNT(vm, 1);
    vm->stack->pop();
}
void script_drop2(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    VM_CHECK_PARAM_COUNT(vm, 2);
    vm->stack->pop(2);
}

void script_depth(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    vm->stack->pushInteger(vm->stack->size());
}
void script_size(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    VM_CHECK_PARAM_COUNT(vm, 1);
    vm->stack->pushInteger(vm->stack->get(-1).size());
}

void script_return(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    vm->env->setError(SCRIPT_ERR_OP_RETURN);
}

void script_hash(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    assert(opcode >= OP_RIPEMD160 && opcode <= OP_HASH256);
    VM_CHECK_PARAM_COUNT(vm, 1);
    xul::hasher* hasher = Hashers::instance().hashers[opcode - OP_RIPEMD160].get();
    assert(hasher);
    std::string s = vm->stack->get(-1);
    hasher->reset();
    hasher->update_string(s);
    std::string digest = hasher->finalize_to_string();
    vm->stack->pop();
    vm->stack->push(digest);
}

void script_checkSig(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    assert(opcode == OP_CHECKSIG || opcode == OP_CHECKSIGVERIFY);
    VM_CHECK_PARAM_COUNT(vm, 2);
    std::string sig = vm->stack->get(-2);
    std::string pubkey = vm->stack->get(-1);
    bool success = vm->env->checker.check(sig, pubkey, vm->env->code);
    vm->stack->pop(2);
    vm->stack->pushBool(success);
    assert(success);
    if (opcode == OP_CHECKSIGVERIFY)
    {
        vm->verify(SCRIPT_ERR_CHECKSIGVERIFY);
    }
}

bool checkMultiSig(ScriptVM* vm, const std::vector<std::string>& pubkeys, const std::vector<std::string>& signatures)
{
    int usedPubkey = 0;
    int passed = 0;
    for (const auto& sig : signatures)
    {
        while (usedPubkey < pubkeys.size())
        {
            const std::string& pubkey = pubkeys[usedPubkey++];
            if (vm->env->checker.check(sig, pubkey, vm->env->code))
            {
                ++passed;
                break;
            }
        }
    }
    assert(passed <= signatures.size());
    return passed >= signatures.size();
}
void script_checkMultiSig(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    assert(opcode == OP_CHECKMULTISIG || opcode == OP_CHECKMULTISIGVERIFY);
    // ([sig ...] num_of_signatures [pubkey ...] num_of_pubkeys -- bool)
    VM_CHECK_PARAM_COUNT(vm, 1);
    int64_t keyCount = 0;
    VM_CHECK_PARAM(vm, vm->getIntegerValue(-1, keyCount));
    VM_CHECK_PARAM(vm, keyCount >= 0 && keyCount < MAX_PUBKEYS_PER_MULTISIG);
    int64_t sigCount = 0;
    VM_CHECK_PARAM_COUNT(vm, 2 + keyCount);
    VM_CHECK_PARAM(vm, vm->getIntegerValue(-2 - keyCount, sigCount));
    VM_CHECK_PARAM(vm, sigCount >= 0 && sigCount <= keyCount);
    std::vector<std::string> pubkeys, signatures;
    VM_CHECK_PARAM(vm, vm->getValues(pubkeys, -2, keyCount));
    VM_CHECK_PARAM(vm, vm->getValues(signatures, -3 - keyCount, sigCount));
    bool success = checkMultiSig(vm, pubkeys, signatures);
    vm->stack->pop(2 + keyCount + sigCount);
    if (vm->stack->size() >= 1)
        vm->stack->pop(1);
    vm->stack->pushBool(success);
    if (opcode == OP_CHECKMULTISIGVERIFY)
    {
        vm->verify(SCRIPT_ERR_CHECKMULTISIGVERIFY);
    }
}


}


ScriptFunctionTable::ScriptFunctionTable()
{
    functions.resize(256);
    for (int i = 0; i < functions.size(); ++i)
        functions[i] = script_reserved;

    for (int i = OP_1; i <= OP_16; ++i)
        functions[i] = script_integer;
    functions[OP_1NEGATE] = script_integer;
    for (int i = OP_0; i < OP_PUSHDATA1; ++i)
        functions[i] = script_push0;
    functions[OP_PUSHDATA1] = script_push1;
    functions[OP_PUSHDATA2] = script_push2;
    functions[OP_PUSHDATA4] = script_push4;

    functions[OP_EQUAL] = script_equal;
    functions[OP_EQUALVERIFY] = script_equal;

    functions[OP_DUP] = script_dup;
    functions[OP_2DUP] = script_dup2;
    functions[OP_3DUP] = script_dup3;
    functions[OP_DROP] = script_drop;
    functions[OP_2DROP] = script_drop2;
    functions[OP_DEPTH] = script_depth;
    functions[OP_SIZE] = script_size;

    for (int i = OP_1ADD; i <= OP_0NOTEQUAL; ++i)
        functions[i] = script_numericUnaryOp;
    for (int i = OP_ADD; i <= OP_MAX; ++i)
        functions[i] = script_numericBinaryOp;
    functions[OP_WITHIN] = script_within;

    for (int i = OP_RIPEMD160; i <= OP_HASH256; ++i)
        functions[i] = script_hash;
    functions[OP_CHECKSIG] = script_checkSig;
    functions[OP_CHECKSIGVERIFY] = script_checkSig;
    functions[OP_CHECKMULTISIG] = script_checkMultiSig;
    functions[OP_CHECKMULTISIGVERIFY] = script_checkMultiSig;

    // controls
    functions[OP_NOP] = script_noop;
    functions[OP_RETURN] = script_return;

    Hashers::instance().init();
    UnaryNumberOps::instance().init();
    BinaryNumberOps::instance().init();
}

void ScriptFunctionTable::eval(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    functions[opcode](vm, opcode, code);
}


}
