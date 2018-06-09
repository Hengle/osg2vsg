#pragma once

#include <vsg/Object.h>

namespace vsg
{
    class Node : public vsg::Object
    {
    public:
        Node();

        virtual void accept(Visitor& visitor) { visitor.apply(*this); }

    protected:
        virtual ~Node();
    };
}
