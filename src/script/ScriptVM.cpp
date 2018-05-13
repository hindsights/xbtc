#include "ScriptVM.hpp"
#include "ScriptFunction.hpp"

#include <xul/io/data_encoding.hpp>
#include <xul/log/log.hpp>


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
    {
        return nullptr;
    }
    return &stack->get(index);
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
    return xul::data_encoding::little_endian().decode(*s, ScriptNumberReader(val));
}


}
