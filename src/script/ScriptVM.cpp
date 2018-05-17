#include "ScriptVM.hpp"
#include "ScriptFunction.hpp"

#include <xul/io/data_encoding.hpp>
#include <xul/log/log.hpp>
#include <xul/text/hex_encoding.hpp>
#include <xul/util/test_case.hpp>
#include <memory>


namespace xbtc {


ScriptVM::ScriptVM(SignatureChecker& chk) : env(new Env(chk)), stack(new Stack())
{
    XUL_LOGGER_INIT("ScriptVM");
    XUL_DEBUG("new");
}

ScriptVM::~ScriptVM()
{
    XUL_DEBUG("delete");
}

bool ScriptVM::eval(const std::string& code)
{
    if (code.empty())
    {
        assert(false);
        return false;
    }
    env->code = code;
    xul::memory_data_input_stream is(&code[0], code.size(), false);
    uint8_t op;
    while (env->errcode == 0 && is.read_uint8(op))
    {
        ScriptFunctionTable::instance().eval(this, op, is);
    }
    return true;
}

const std::string* ScriptVM::getValue(int index)
{
    if (index < -stack->size() || index >= stack->size())
        return nullptr;
    return &stack->get(index);
}

bool ScriptVM::getValues(std::vector<std::string>& vals, int index, int count)
{
    if (index < -stack->size() || index >= stack->size())
        return false;
    if (index < 0 && index - count < -stack->size())
        return false;
    if (index >= 0 && index + count >= stack->size())
        return false;
    for (int i = 0; i < count; ++i)
    {
        vals.push_back(stack->get(index >= 0 ? index + i : index - i));
    }
    return true;
}

bool ScriptVM::getBoolValue(int index, bool& val)
{
    const std::string* s = getValue(index);
    if (!s)
        return false;
    val = false;
    for (int i = 0; i < s->size(); i++)
    {
        uint8_t ch = (*s)[i];
        if (ch != 0)
        {
            // Can be negative zero
            val = (i != s->size()-1 || ch != 0x80);
            break;
        }
    }
    return true;
}

bool ScriptVM::getIntegerValue(int index, int64_t& val)
{
    const std::string* s = getValue(index);
    if (!s)
        return false;
    return ScriptUtils::decodeScriptNumber(val, *s);
}

void ScriptVM::verify(int errcode)
{
    bool success;
    bool ret = getBoolValue(-1, success);
    assert(ret);
    if (success)
        stack->pop();
    else
        env->setError(errcode);
}


}

#ifdef XUL_RUN_TEST

#include "data/Transaction.hpp"

namespace xbtc {

class ScriptVMTestCase : public xul::test_case
{
public:
    virtual void run()
    {
//        testCheckMultiSig();
    }
    void testCheckMultiSig()
    {
        Transaction tx;
        tx.inputs.resize(2);
        tx.inputs[0].previousOutput.hash.parse("011E6BAFBF9584B56E7F0CDC3F98DD50E6A9715250751AE8F9316266A82AA3C0");
        tx.inputs[0].previousOutput.index = 1;
        tx.inputs[0].signatureScript = xul::hex_encoding::decode("00483045022100F48ACE0252EABEA35411322FB783728F2CD1F358CD205169A0C1BADD04894A2602200F94F3E1A8BCE6E9FBB00755D91BCF6DF650A04AA65AF94DCF634D7674D5FE7C01493046022100EFF2DC36C9E587C6044AA2FE1FA7E92961FA59412BF25599EC8D7FC4A7390109022100DA20BB91638C3B3D6091934F38C9BC0DF8A711C1DCF43B0B7F2B06A7DD2F1D050148304502204ABB926C3D7C971E369035E59241FA4535543F595F6D51BFF59644D4170E28AE022100ECBBA2194B6ED940FB5A843A116B9D521FE80B27003A9C039978C05295CBAA4C01");
        tx.inputs[1].previousOutput.hash.parse("44F6C3352F9A1AA3A204AC9E684D067CA4C28D9E68B040F241F327CFED1FCD55");
        tx.inputs[1].previousOutput.index = 0;
        tx.inputs[1].signatureScript = xul::hex_encoding::decode("4830450220592C2E4B2A300F044ADA457639D8B4034800580B3767D6861D616BA238FA0271022100B4DD62112068C1A650B97501D6F61795AD7AC7DAEEB4C84C9507282E3926235001210379E40B626F66E5229E901EDA176DA1373A358BD4B5D9C891CAA7CE8BFDC21159");

        tx.outputs.resize(2);
        tx.outputs[0].value = 49100000;
        tx.outputs[0].scriptPublicKey = xul::hex_encoding::decode("76A914DB79F1895D4F2000E1026B15798FE42FF8C2DD7488AC");
        tx.outputs[1].value = 49100000;
        tx.outputs[1].scriptPublicKey = xul::hex_encoding::decode("5321035ED62CA224299B3F9C086BCB1BA8540A2EC038D79735094FAEFD5914F9BC9CC121039F9C663D8E6ECF447C085855035FF7986799839C6A9D02BE9FA17DE11B38705A2103A0AF8A3AFD0E9203EBBFAEB7FF02AA476C19474BB330648376188770E71CEE1053AE");
        testHexScript(tx, 0, "5321035ED62CA224299B3F9C086BCB1BA8540A2EC038D79735094FAEFD5914F9BC9CC121039F9C663D8E6ECF447C085855035FF7986799839C6A9D02BE9FA17DE11B38705A2103A0AF8A3AFD0E9203EBBFAEB7FF02AA476C19474BB330648376188770E71CEE1053AE");
    }
    void testHexScript(Transaction& tx, int inputIndex, const std::string& pubkeyText)
    {
        std::shared_ptr<SignatureChecker> checker(createSignatureChecker(&tx, inputIndex));
        ScriptVM vm(*checker);
        std::string pubkey;
        assert(xul::hex_encoding::decode(pubkey, pubkeyText));
        assert(vm.eval(tx.inputs[inputIndex].signatureScript));
        assert(vm.eval(pubkey));
        bool success;
        assert(vm.getBoolValue(-1, success) && success);
    }
};

XUL_TEST_SUITE_REGISTRATION(ScriptVMTestCase);

}

#endif
