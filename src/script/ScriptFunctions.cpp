#include "ScriptVM.hpp"
#include "ScriptFunction.hpp"
#include "util/Hasher.hpp"

#include <xul/io/data_input_stream.hpp>
#include <xul/crypto/hasher.hpp>
#include <xul/util/singleton.hpp>
#include <xul/std/strings.hpp>
#include <xul/log/log.hpp>


namespace xbtc {


const std::string vTrue(1, OP_TRUE);
const std::string vFalse;


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


void script_reserved(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    XUL_WARN(vm, "script_reserved " << xul::strings::format("opcode=0x%02X", opcode));
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
    assert(vm->stack->size() >= 2);
    const std::string& v1 = vm->stack->get(-1);
    const std::string& v2 = vm->stack->get(-2);
    bool ret = (v1 == v2);
    vm->stack->pop(2);
    vm->stack->pushBool(ret);
    if (opcode == OP_EQUALVERIFY)
    {
        if (ret)
            vm->stack->pop();
        else
            vm->env->setError(SCRIPT_ERR_EQUALVERIFY);
    }
}


void script_dup(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    assert(vm->stack->size() >= 1);
    vm->stack->dup();
}
void script_dup2(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    assert(vm->stack->size() >= 2);
    vm->stack->dup(2);
}
void script_dup3(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    assert(vm->stack->size() >= 3);
    vm->stack->dup(3);
}

void script_drop(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    assert(vm->stack->size() >= 1);
    vm->stack->pop();
}
void script_drop2(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    assert(vm->stack->size() >= 2);
    vm->stack->pop(2);
}

void script_return(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    vm->env->setError(SCRIPT_ERR_OP_RETURN);
}

void script_hash(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    assert(opcode >= OP_RIPEMD160 && opcode <= OP_HASH256);
    assert(vm->stack->size() >= 1);
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
    assert(vm->stack->size() >= 2);
    std::string sig = vm->stack->get(-2);
    std::string pubkey = vm->stack->get(-1);
    bool success = vm->env->checker.check(sig, pubkey, vm->env->code);
    vm->stack->pop(2);
    vm->stack->pushBool(success);
    if (opcode == OP_CHECKSIGVERIFY)
    {
        if (success)
            vm->stack->pop();
        else
            vm->env->setError(SCRIPT_ERR_CHECKSIGVERIFY);
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

    for (int i = OP_RIPEMD160; i <= OP_HASH256; ++i)
        functions[i] = script_hash;
    functions[OP_CHECKSIG] = script_checkSig;
    functions[OP_CHECKSIGVERIFY] = script_checkSig;

    // controls
    functions[OP_NOP] = script_noop;
    functions[OP_RETURN] = script_return;

    Hashers::instance().init();
}

void ScriptFunctionTable::eval(ScriptVM* vm, uint8_t opcode, xul::data_input_stream& code)
{
    functions[opcode](vm, opcode, code);
}


}
