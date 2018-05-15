#pragma once

#include "Script.hpp"
#include <xul/util/singleton.hpp>

#include <string>
#include <vector>
#include <stdint.h>
#include <assert.h>


namespace xbtc {


extern const std::string vTrue;
extern const std::string vFalse;


class ScriptVM;
typedef void (*ScriptFunctionType)(ScriptVM*, uint8_t, xul::data_input_stream&);

class ScriptFunctionTable : public xul::singleton<ScriptFunctionTable>
{
public:
    std::vector<ScriptFunctionType> functions;

public:
    ScriptFunctionTable();
    void eval(ScriptVM* vm, uint8_t op, xul::data_input_stream& code);
};


class Stack
{
public:
    std::vector<std::string> vars;

    void push(const std::string& s)
    {
        vars.push_back(s);
    }
    void pushInteger(int64_t val)
    {
        vars.push_back(ScriptUtils::encodeScriptNumber(val));
    }
    void push(int index)
    {
        push(get(index));
    }
    void dup()
    {
        push(-1);
    }
    void dup(int count)
    {
        for (int i = 0; i < count; ++i)
        {
            push(-i*2 - 1);
        }
    }
    void pop()
    {
        assert(!vars.empty());
        vars.pop_back();
    }
    void pop(int n)
    {
        assert(vars.size() >= n);
        for (int i = 0; i < n; ++i)
        {
            vars.pop_back();
        }
    }
    const std::string& get(int index) const
    {
        if (index < 0)
            index += vars.size();
        assert(index >= 0 && index < vars.size());
        return vars[index];
    }
    std::string& get(int index)
    {
        if (index < 0)
            index += vars.size();
        assert(index >= 0 && index < vars.size());
        return vars[index];
    }
    int size() const { return vars.size(); }
    void pushBool(bool val)
    {
        push(val ? vTrue : vFalse);
    }
};


class Env
{
public:
    int errcode;
    std::string code;
    SignatureChecker& checker;

    explicit Env(SignatureChecker& chk) : errcode(0), checker(chk)
    {
    }
    void setError(int err)
    {
        errcode = err;
    }
};


}
