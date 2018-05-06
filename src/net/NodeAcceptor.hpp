#pragma once

#include <xul/lang/object.hpp>
#include <stdint.h>


namespace xbtc {


class NodeAcceptor : public xul::object
{
public:
    virtual int open(int port) = 0;
    virtual void onTick(int64_t times) = 0;
};


class PPLiteModule;
NodeAcceptor* createNodeAcceptor(PPLiteModule* module);


}
