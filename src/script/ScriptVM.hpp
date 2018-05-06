#pragma once

#include <xul/log/log.hpp>
#include <memory>
#include <string>
#include <stdint.h>


namespace xbtc {


class SignatureChecker;
class Env;
class Stack;


class ScriptVM
{
public:
    XUL_LOGGER_DEFINE();
    std::unique_ptr<Env> env;
    std::unique_ptr<Stack> stack;

    explicit ScriptVM(SignatureChecker& chk);
    ~ScriptVM();
    bool eval(const std::string& code);
    const std::string* getValue(int index);
    bool getBoolValue(int index, bool& val);
    bool getIntegerValue(int index, int64_t& val);
};


}
