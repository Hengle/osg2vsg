#include <vsg/all.h>

#include <osg/ArgumentParser>
#include <osg/PagedLOD>
#include <osgDB/ReadFile>
#include <osgUtil/Optimizer>
#include <osgUtil/MeshOptimizers>

#include <osg2vsg/GeometryUtils.h>
#include <osg2vsg/ShaderUtils.h>
#include <osg2vsg/SceneBuilder.h>
#include <osg2vsg/Optimize.h>

#include "ConvertToVsg.h"

using namespace osg2vsg;

vsg::ref_ptr<vsg::BindGraphicsPipeline> ConvertToVsg::getOrCreateBindGraphicsPipeline(uint32_t shaderModeMask, uint32_t geometryMask)
{
    MaskPair masks(shaderModeMask, geometryMask);
    if (auto itr = pipelineMap.find(masks); itr != pipelineMap.end()) return itr->second;

    auto bindGraphicsPipeline = createBindGraphicsPipeline(shaderModeMask, geometryMask, vertexShaderPath, fragmentShaderPath);
    pipelineMap[masks] = bindGraphicsPipeline;
    return bindGraphicsPipeline;
}

vsg::ref_ptr<vsg::BindDescriptorSet> ConvertToVsg::getOrCreateBindDescriptorSet(uint32_t shaderModeMask, uint32_t geometryMask, osg::StateSet* stateset)
{
    MasksAndState masksAndState(shaderModeMask, geometryMask, stateset);
    if (auto itr = bindDescriptorSetMap.find(masksAndState); itr != bindDescriptorSetMap.end())
    {
        // std::cout<<"reusing bindDescriptorSet "<<itr->second.get()<<std::endl;
        return itr->second;
    }

    MaskPair masks(shaderModeMask, geometryMask);

    auto bindGraphicsPipeline = pipelineMap[masks];
    if (!bindGraphicsPipeline) return {};

    auto pipeline = bindGraphicsPipeline->getPipeline();
    if (!pipeline) return {};

    auto pipelineLayout = pipeline->getPipelineLayout();
    if (!pipelineLayout) return {};

    auto descriptorSet = createVsgStateSet(pipelineLayout->getDescriptorSetLayouts(), stateset, shaderModeMask);
    if (!descriptorSet) return {};

    // std::cout<<"   We have descriptorSet "<<descriptorSet<<std::endl;

    auto bindDescriptorSet = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, descriptorSet);

    bindDescriptorSetMap[masksAndState] = bindDescriptorSet;

    return bindDescriptorSet;
}

vsg::Path ConvertToVsg::mapFileName(const std::string& filename)
{
    if (auto itr = filenameMap.find(filename); itr != filenameMap.end())
    {
        return itr->second;
    }

    vsg::Path vsg_filename = vsg::removeExtension(filename) + "." + extension;

    filenameMap[filename] = vsg_filename;

    return vsg_filename;
}

void ConvertToVsg::optimize(osg::Node* osg_scene)
{
    osgUtil::IndexMeshVisitor imv;
    #if OSG_MIN_VERSION_REQUIRED(3,6,4)
    imv.setGenerateNewIndicesOnAllGeometries(true);
    #endif
    osg_scene->accept(imv);
    imv.makeMesh();

    osgUtil::VertexCacheVisitor vcv;
    osg_scene->accept(vcv);
    vcv.optimizeVertices();

    osgUtil::VertexAccessOrderVisitor vaov;
    osg_scene->accept(vaov);
    vaov.optimizeOrder();

    osgUtil::Optimizer optimizer;
    optimizer.optimize(osg_scene, osgUtil::Optimizer::DEFAULT_OPTIMIZATIONS);

    osg2vsg::OptimizeOsgBillboards optimizeBillboards;
    osg_scene->accept(optimizeBillboards);
    optimizeBillboards.optimize();
}

vsg::ref_ptr<vsg::Node> ConvertToVsg::convert(osg::Node* node)
{
    root = nullptr;

    if (auto itr = nodeMap.find(node); itr != nodeMap.end())
    {
        root = itr->second;
    }
    else
    {
        if (node) node->accept(*this);

        nodeMap[node] = root;
    }

    return root;
}

vsg::ref_ptr<vsg::Data> ConvertToVsg::copy(osg::Array* src_array)
{
    if (!src_array) return {};

    switch(src_array->getType())
    {
        case(osg::Array::ByteArrayType): return {};
        case(osg::Array::ShortArrayType): return {};
        case(osg::Array::IntArrayType): return {};

        case(osg::Array::UByteArrayType): return copyArray<vsg::ubyteArray>(src_array);
        case(osg::Array::UShortArrayType): return copyArray<vsg::ushortArray>(src_array);
        case(osg::Array::UIntArrayType): return copyArray<vsg::uintArray>(src_array);

        case(osg::Array::FloatArrayType): return copyArray<vsg::floatArray>(src_array);
        case(osg::Array::DoubleArrayType): return copyArray<vsg::doubleArray>(src_array);

        case(osg::Array::Vec2bArrayType): return {};
        case(osg::Array::Vec3bArrayType): return {};
        case(osg::Array::Vec4bArrayType): return {};

        case(osg::Array::Vec2sArrayType): return {};
        case(osg::Array::Vec3sArrayType): return {};
        case(osg::Array::Vec4sArrayType): return {};

        case(osg::Array::Vec2iArrayType): return {};
        case(osg::Array::Vec3iArrayType): return {};
        case(osg::Array::Vec4iArrayType): return {};

        case(osg::Array::Vec2ubArrayType): return copyArray<vsg::ubvec2Array>(src_array);
        case(osg::Array::Vec3ubArrayType): return copyArray<vsg::ubvec3Array>(src_array);
        case(osg::Array::Vec4ubArrayType): return copyArray<vsg::ubvec4Array>(src_array);

        case(osg::Array::Vec2usArrayType): return copyArray<vsg::usvec2Array>(src_array);
        case(osg::Array::Vec3usArrayType): return copyArray<vsg::usvec3Array>(src_array);
        case(osg::Array::Vec4usArrayType): return copyArray<vsg::usvec4Array>(src_array);

        case(osg::Array::Vec2uiArrayType): return copyArray<vsg::uivec2Array>(src_array);
        case(osg::Array::Vec3uiArrayType): return copyArray<vsg::usvec3Array>(src_array);
        case(osg::Array::Vec4uiArrayType): return copyArray<vsg::usvec4Array>(src_array);

        case(osg::Array::Vec2ArrayType): return copyArray<vsg::vec2Array>(src_array);
        case(osg::Array::Vec3ArrayType): return copyArray<vsg::vec3Array>(src_array);
        case(osg::Array::Vec4ArrayType): return copyArray<vsg::vec4Array>(src_array);

        case(osg::Array::Vec2dArrayType): return copyArray<vsg::dvec2Array>(src_array);
        case(osg::Array::Vec3dArrayType): return copyArray<vsg::dvec2Array>(src_array);
        case(osg::Array::Vec4dArrayType): return copyArray<vsg::dvec2Array>(src_array);

        case(osg::Array::MatrixArrayType): return copyArray<vsg::mat4Array>(src_array);
        case(osg::Array::MatrixdArrayType): return copyArray<vsg::dmat4Array>(src_array);

        case(osg::Array::QuatArrayType): return {};

        case(osg::Array::UInt64ArrayType): return {};
        case(osg::Array::Int64ArrayType): return {};
        default: return {};
    }
}

uint32_t ConvertToVsg::calculateShaderModeMask()
{
    if (statestack.empty()) return osg2vsg::ShaderModeMask::NONE;

    auto& statepair = getStatePair();

    return osg2vsg::calculateShaderModeMask(statepair.first) | osg2vsg::calculateShaderModeMask(statepair.second);
}



void ConvertToVsg::apply(osg::Geometry& geometry)
{
    ScopedPushPop spp(*this, geometry.getStateSet());

    uint32_t geometryMask = (osg2vsg::calculateAttributesMask(&geometry) | overrideGeomAttributes) & supportedGeometryAttributes;
    uint32_t shaderModeMask = (calculateShaderModeMask() | overrideShaderModeMask | nodeShaderModeMasks) & supportedShaderModeMask;

    // std::cout<<"Have geometry with "<<statestack.size()<<" shaderModeMask="<<shaderModeMask<<", geometryMask="<<geometryMask<<std::endl;

    auto stategroup = vsg::StateGroup::create();

    auto bindGraphicsPipeline = getOrCreateBindGraphicsPipeline(shaderModeMask, geometryMask);
    if (bindGraphicsPipeline) stategroup->add(bindGraphicsPipeline);

    auto vsg_geometry = osg2vsg::convertToVsg(&geometry, geometryMask, geometryTarget);

    if (!statestack.empty())
    {
        auto stateset = getStatePair().second;
        //std::cout<<"   We have stateset "<<stateset<<", descriptorSetLayouts.size() = "<<descriptorSetLayouts.size()<<", "<<shaderModeMask<<std::endl;
        if (stateset)
        {
            auto bindDescriptorSet = getOrCreateBindDescriptorSet(shaderModeMask, geometryMask, stateset);
            if (bindDescriptorSet) stategroup->add(bindDescriptorSet);
        }
    }

    stategroup->addChild(vsg_geometry);

    root = stategroup;
}

void ConvertToVsg::apply(osg::Group& group)
{
    if (osgTerrain::TerrainTile* tile = dynamic_cast<osgTerrain::TerrainTile*>(&group); tile)
    {
        apply(*tile);
        return;
    }

    ScopedPushPop spp(*this, group.getStateSet());

    auto vsg_group = vsg::Group::create();

    //vsg_group->setValue("class", group.className());

    for(unsigned int i=0; i<group.getNumChildren(); ++i)
    {
        auto child = group.getChild(i);
        if (auto vsg_child = convert(child); vsg_child)
        {
            vsg_group->addChild(vsg_child);
        }
    }

    root = vsg_group;
}

void ConvertToVsg::apply(osg::MatrixTransform& transform)
{
    ScopedPushPop spp(*this, transform.getStateSet());

    auto vsg_transform = vsg::MatrixTransform::create();
    vsg_transform->setMatrix(osg2vsg::convert(transform.getMatrix()));

    for(unsigned int i=0; i<transform.getNumChildren(); ++i)
    {
        auto child = transform.getChild(i);
        if (auto vsg_child = convert(child); vsg_child)
        {
            vsg_transform->addChild(vsg_child);
        }
    }

    root = vsg_transform;

}

void ConvertToVsg::apply(osg::CoordinateSystemNode& cs)
{
    apply((osg::Group&)cs);

    if (root) root->setValue("class", "CoordinateSystemNode");
}

void ConvertToVsg::apply(osg::Billboard& billboard)
{
    ScopedPushPop spp(*this, billboard.getStateSet());

    if (billboardTransform)
    {
        nodeShaderModeMasks = BILLBOARD;
    }
    else
    {
        nodeShaderModeMasks = BILLBOARD | SHADER_TRANSLATE;
    }

    auto vsg_group = vsg::Group::create();

    if (nodeShaderModeMasks & SHADER_TRANSLATE)
    {
        using Positions = std::vector<osg::Vec3>;
        using ChildPositions = std::map<osg::Drawable*, Positions>;
        ChildPositions childPositions;

        for(unsigned int i=0; i<billboard.getNumDrawables(); ++i)
        {
            childPositions[billboard.getDrawable(i)].push_back(billboard.getPosition(i));
        }

        struct ComputeBillboardBoundingBox : public osg::Drawable::ComputeBoundingBoxCallback
        {
            Positions positions;

            ComputeBillboardBoundingBox(const Positions& in_positions) : positions(in_positions) {}

            virtual osg::BoundingBox computeBound(const osg::Drawable& drawable) const
            {
                const osg::Geometry* geom = drawable.asGeometry();
                const osg::Vec3Array* vertices = geom ? dynamic_cast<const osg::Vec3Array*>(geom->getVertexArray()) : nullptr;
                if (vertices)
                {
                    osg::BoundingBox local_bb;
                    for(auto vertex : *vertices)
                    {
                        local_bb.expandBy(vertex);
                    }

                    osg::BoundingBox world_bb;
                    for(auto position : positions)
                    {
                        world_bb.expandBy(local_bb._min + position);
                        world_bb.expandBy(local_bb._max + position);
                    }

                    return world_bb;
                }

                return osg::BoundingBox();
            }
        };

        for(auto&[child, positions] : childPositions)
        {
            osg::Geometry* geometry = child->asGeometry();
            if (geometry)
            {
                geometry->setComputeBoundingBoxCallback(new ComputeBillboardBoundingBox(positions));

                osg::ref_ptr<osg::Vec3Array> positionArray = new osg::Vec3Array(positions.begin(), positions.end());
                positionArray->setBinding(osg::Array::BIND_OVERALL);
                geometry->setVertexAttribArray(7, positionArray);

                root = nullptr;

                geometry->accept(*this);

                if (root) vsg_group->addChild(root);
            }
        }
    }
    else
    {
        for(unsigned int i=0; i<billboard.getNumDrawables(); ++i)
        {
            auto translate = osg::Matrixd::translate(billboard.getPosition(i));

            auto vsg_transform = vsg::MatrixTransform::create();
            vsg_transform->setMatrix(osg2vsg::convert(translate));

            auto vsg_child = convert(billboard.getDrawable(i));

            if (vsg_child)
            {
                vsg_transform->addChild(vsg_child);
                vsg_group->addChild(vsg_transform);
            }
        }
    }

    if (vsg_group->getNumChildren()==9) root = nullptr;
    else if (vsg_group->getNumChildren()==1) root = vsg_group->getChild(0);
    else root = vsg_group;

    nodeShaderModeMasks = NONE;
}

void ConvertToVsg::apply(osg::LOD& lod)
{
    auto vsg_lod = vsg::LOD::create();

    const osg::BoundingSphere& bs = lod.getBound();
    osg::Vec3d center = (lod.getCenterMode()==osg::LOD::USER_DEFINED_CENTER) ? lod.getCenter() : bs.center();
    double radius = (lod.getRadius()>0.0) ? lod.getRadius() : bs.radius();

    vsg_lod->setBound(vsg::dsphere(center.x(), center.y(), center.z(), radius));

    unsigned int numChildren = std::min(lod.getNumChildren(), lod.getNumRanges());

    const double pixel_ratio = 1.0/1080.0;
    const double angle_ratio = 1.0/osg::DegreesToRadians(30.0); // assume a 60 fovy for reference

    // build a map of minimum screen ration to child
    std::map<double, vsg::ref_ptr<vsg::Node>> ratioChildMap;
    for(unsigned int i = 0; i < numChildren; ++i)
    {
        if (auto vsg_child = convert(lod.getChild(i)); vsg_child)
        {
            double minimumScreenHeightRatio = (lod.getRangeMode()==osg::LOD::DISTANCE_FROM_EYE_POINT) ?
                (atan2(radius, static_cast<double>(lod.getMaxRange(i))) * angle_ratio) :
                (lod.getMinRange(i) * pixel_ratio);

            ratioChildMap[minimumScreenHeightRatio] = vsg_child;
        }
    }

    // add to vsg::LOD in reverse order - highest level of detail first
    for(auto itr = ratioChildMap.rbegin(); itr != ratioChildMap.rend(); ++itr)
    {
        vsg_lod->addChild(vsg::LOD::LODChild{itr->first, itr->second});
    }

    root = vsg_lod;
}

void ConvertToVsg::apply(osg::PagedLOD& plod)
{
    auto vsg_lod = vsg::PagedLOD::create();

    const osg::BoundingSphere& bs = plod.getBound();
    osg::Vec3d center = (plod.getCenterMode()==osg::LOD::USER_DEFINED_CENTER) ? plod.getCenter() : bs.center();
    double radius = (plod.getRadius()>0.0) ? plod.getRadius() : bs.radius();

    vsg_lod->setBound(vsg::dsphere(center.x(), center.y(), center.z(), radius));

    unsigned int numChildren = plod.getNumChildren();
    unsigned int numRanges = plod.getNumRanges();

    const double pixel_ratio = 1.0/1080.0;
    const double angle_ratio = 1.0/osg::DegreesToRadians(30.0); // assume a 60 fovy for reference

    struct CompareChild
    {
        bool operator() (const vsg::PagedLOD::PagedLODChild& lhs, const vsg::PagedLOD::PagedLODChild& rhs) const
        {
            return lhs.minimumScreenHeightRatio > rhs.minimumScreenHeightRatio;
        }
    };

    using Children = std::set<vsg::PagedLOD::PagedLODChild, CompareChild>;
    Children children;

    for(unsigned int i = 0; i < numRanges; ++i)
    {
        vsg::ref_ptr<vsg::Node> vsg_child;
        if (i<numChildren) vsg_child = convert(plod.getChild(i));

        double minimumScreenHeightRatio = (plod.getRangeMode()==osg::LOD::DISTANCE_FROM_EYE_POINT) ?
            (atan2(radius, static_cast<double>(plod.getMaxRange(i))) * angle_ratio) :
            (plod.getMinRange(i) * pixel_ratio);

        auto osg_filename = plod.getFileName(i);
        auto vsg_filename = osg_filename.empty() ? vsg::Path() : mapFileName(osg_filename);

        children.insert(vsg::PagedLOD::PagedLODChild{minimumScreenHeightRatio, vsg_filename, vsg_child, vsg_child});
    }

    // add to vsg::LOD in reverse order - highest level of detail first
    for(auto& child : children)
    {
        vsg_lod->addChild(child);
    }

    root = vsg_lod;
}

void ConvertToVsg::apply(osgTerrain::TerrainTile& tile)
{
    root = nullptr;

    tile.traverse(*this);
}
