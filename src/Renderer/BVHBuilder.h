#pragma once
#include "timCore/type.h"
#include <vector>

#include "Shaders/primitive_cpp.glsl"
#include "BVHGeometry.h"

namespace tim
{
    struct Primitive
    {
        Primitive(const Sphere& _sphere, const Material& _mat) : type{ Primitive_Sphere }, material{ _mat }, m_sphere{ _sphere } {}
        Primitive(const Box& _box, const Material& _mat) : type{ Primitive_AABB }, material{ _mat }, m_aabb{ _box } {}

        u32 type = 0;
        Material material;
        union
        {
            Sphere  m_sphere = { {0,0,0}, 1, 1 };
            Box     m_aabb;
        };
    };

    struct Light
    {
        Light(const SphereLight& _light) : type{ Light_Sphere }, m_sphere{ _light } {}
        Light(const AreaLight& _light) : type{ Light_Area }, m_area{ _light } {}

        u32 type = 0;
        union
        {
            SphereLight m_sphere;
            AreaLight   m_area;
        };
    };

    class BVHBuilder
    {
    public:
        static Material createLambertianMaterial(vec3 _color);
        static Material createEmissiveMaterial(vec3 _color);
        static Material createMirrorMaterial(vec3 _color, float _mirrorness);
        static Material createTransparentMaterial(vec3 _color, float _refractionIndice, float _reflectivity);

        BVHBuilder() {}

        void addSphere(const Sphere&, const Material& _mat = createLambertianMaterial({ 0.7f, 0.7f, 0.7f }));
        void addBox(const Box& _box, const Material& _mat = createLambertianMaterial({ 0.7f, 0.7f, 0.7f }));
        void addSphereLight(const SphereLight& _light);
        void addAreaLight(const AreaLight& _light);
        void build(u32 _maxDepth, u32 _maxObjPerNode, const Box& _sceneSize);

        u32 getPrimitivesCount() const { return u32(m_objects.size()); }
        u32 getLightsCount() const { return u32(m_lights.size()); }
        u32 getNodesCount() const { return u32(m_nodes.size()); }

        u32 getBvhGpuSize() const;

        // return offset to root node + offset to first primitive list of leafs, offset 0 is for primitive data
        void fillGpuBuffer(void* _data, uvec2& _primitiveOffsetRange, uvec2& _materialOffsetRange, uvec2& _lightOffsetRange, uvec2& _nodeOffsetRange, uvec2& m_leafDataOffsetRange);

    private:
        struct Node;
        using ObjectIt = std::vector<u32>::iterator;
        void addObjectsRec(u32 _depth, ObjectIt _objectsBegin, ObjectIt _objectsEnd, Node* _curNode);

        template<typename Fun1, typename Fun2>
        void searchBestSplit(Node* _curNode, ObjectIt _objectsBegin, ObjectIt _objectsEnd, const Fun1& _computeStep, const Fun2& _computeFixedStep,
            Box& _leftBox, Box& _rightBox, size_t& _numObjInLeft, size_t& _numObjInRight) const;

        void packNodeData(PackedBVHNode* _outNode, const BVHBuilder::Node& _node, u32 _leafDataOffset);

    private:
        const u32 m_bufferAlignment = 32;
        u32 m_maxDepth = 0, m_maxObjPerNode = 0;

        struct Node
        {
            Box extent;
            Node* parent = nullptr;
            Node* sibling = nullptr;
            Node* left = nullptr;
            Node* right = nullptr;
            std::vector<u32> primitiveList; // only if leaf
            std::vector<u32> lightList; // only if leaf
        };

        std::vector<Node> m_nodes;
        std::vector<Primitive> m_objects;
        std::vector<Light> m_lights;
    };
}