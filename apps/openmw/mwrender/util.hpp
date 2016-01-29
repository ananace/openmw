#ifndef OPENMW_MWRENDER_UTIL_H
#define OPENMW_MWRENDER_UTIL_H

#include <osg/Callback>
#include <osg/ref_ptr>
#include <string>

namespace osg
{
    class Node;
}

namespace Resource
{
    class ResourceSystem;
}

namespace MWRender
{
    void overrideTexture(const std::string& texture, Resource::ResourceSystem* resourceSystem, osg::ref_ptr<osg::Node> node);

    // Node callback to entirely skip the traversal.
    class NoTraverseCallback : public osg::NodeCallback
    {
    public:
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            // no traverse()
        }
    };
}

#endif
