#pragma once

#include <xul/lang/object.hpp>
#include <stdint.h>

namespace xul {
    class structured_writer;
}


namespace xbtc {


class AppInfo;
class NodeManager;

class NodeConnector : public xul::object
{
public:
    virtual void schedule() = 0;
    virtual void stop() = 0;
    virtual void onTick(int64_t times) = 0;
    virtual void dump(xul::structured_writer* writer, int level) const = 0;
};


NodeConnector* createNodeConnector(NodeManager* mgr);


}
