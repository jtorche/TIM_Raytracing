#pragma once
#include "timCore/type.h"
#include <vector>

#include "Shaders/primitive_cpp.glsl"
#include "BVHGeometry.h"
#include <mutex>

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
            Sphere      m_sphere = { {0,0,0}, 1, 1 };
            Box         m_aabb;
        };
    };

    struct TrianglePrimitive
    {
        Triangle m_triangle;
        Material m_material;
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

    struct BlasInstance
    {
        u32 blasId;
        u32 matId;
        Box aabb;
    };

    enum class CollisionType { Disjoint, Intersect, Contained };

    class BVHBuilder
    {
    public:
        static Material createLambertianMaterial(vec3 _color);
        static Material createPbrMaterial(vec3 _color, float _metalness = 0);
        static Material createEmissiveMaterial(vec3 _color);
        static Material createMirrorMaterial(vec3 _color, float _mirrorness);
        static Material createTransparentMaterial(vec3 _color, float _refractionIndice, float _reflectivity);
        static void setTextureMaterial(Material& _mat, u32 _texture0, u32 _texture1);

        BVHBuilder(const BVHGeometry& _geometry) : m_geometryBuffer{ _geometry } {}

        void dumpStats() const;

        void addSphere(const Sphere&, const Material& _mat = createLambertianMaterial({ 0.7f, 0.7f, 0.7f }));
        void addBox(const Box& _box, const Material& _mat = createLambertianMaterial({ 0.7f, 0.7f, 0.7f }));
        void addTriangle(const BVHGeometry::TriangleData& _triangle, const Material& _mat = createLambertianMaterial({ 0.7f, 0.7f, 0.7f }));
        void addTriangleList(u32 _vertexOffset, u32 _numTriangle, const u32 * _indexData, const Material& _mat = createLambertianMaterial({ 0.7f, 0.7f, 0.7f }));
        void addBlas(std::unique_ptr<BVHBuilder> _blas);
        void addSphereLight(const SphereLight& _light);
        void addAreaLight(const AreaLight& _light);

        void buildBlas(u32 _maxObjPerNode);
        void build(u32 _maxDepth, u32 _maxObjPerNode, bool _useMultipleThreads);
        void computeSceneAABB();

        Box getAABB() const { return m_aabb; }
        u32 getPrimitivesCount() const { return u32(m_objects.size()); }
        u32 getTrianglesCount() const { return u32(m_triangles.size()); }
        u32 getBlasInstancesCount() const { return u32(m_blasInstances.size()); }
        u32 getLightsCount() const { return u32(m_lights.size()); }
        u32 getNodesCount() const { return u32(m_nodes.size()); }

        u32 getBvhGpuSize() const;

        // return offset to root node + offset to first primitive list of leafs, offset 0 is for primitive data
        void fillGpuBuffer(void* _data, uvec2& _triangleOffsetRange, uvec2& _primitiveOffsetRange, uvec2& _materialOffsetRange, uvec2& _lightOffsetRange, uvec2& _nodeOffsetRange, uvec2& m_leafDataOffsetRange, uvec2& _blasOffsetRange);

        // Helpers
        void fillTriangles(byte* _outData) const;

    private:
        void addTriangle(const BVHGeometry::TriangleData& _triangle, u32 _materialId);

        struct Node;
        using ObjectIt = std::vector<u32>::iterator;
        void addObjectsRec(u32 _depth, ObjectIt _objectsBegin, ObjectIt _objectsEnd, 
                                       ObjectIt _trianglesBegin, ObjectIt _trianglesEnd, 
                                       ObjectIt _blasBegin, ObjectIt _blasEnd,
                                       Node* _curNode, bool _useMultipleThreads);

        template<typename Fun1, typename Fun2>
        void searchBestSplit(Node* _curNode, ObjectIt _objectsBegin, ObjectIt _objectsEnd, ObjectIt _trianglesBegin, ObjectIt _trianglesEnd, ObjectIt _blasBegin, ObjectIt _blasEnd,
                             const Fun1& _movingAxis, const Fun2& _fixedAxis, Box& _leftBox, Box& _rightBox, u32& _numObjInLeft, u32& _numObjInRight) const;

        void packNodeData(PackedBVHNode* _outNode, const BVHBuilder::Node& _node, u32 _leafDataOffset);

        Box getAABB(const Triangle& _triangle) const;
        Box getAABB(const Primitive& _prim) const;
        CollisionType triangleBoxCollision(const Triangle& _triangle, const Box& _box) const;
        CollisionType primitiveBoxCollision(const Primitive& _prim, const Box& _box) const;
        CollisionType primitiveSphereCollision(const Primitive& _prim, const Sphere& _sphere) const;

    private:
        const u32 m_bufferAlignment = 32;
        u32 m_maxDepth = 0, m_maxObjPerNode = 0;
        std::mutex m_mutex;

        // stats for leaf node
        struct Stats
        {
            u32 numLeafs = 0;
            u32 maxTriangle = 0;
            u32 maxBlas = 0;
            float meanTriangle = 0;
            float meanBlas = 0;
            float meanDepth = 0;
        };
        Stats m_stats;

        const BVHGeometry& m_geometryBuffer;

        struct Node
        { 
            Node(size_t _nid) : nid{ u32(_nid) } {}

            u32 nid = u32(-1);
            Box extent;
            Node* parent = nullptr;
            Node* sibling = nullptr;
            Node* left = nullptr;
            Node* right = nullptr;
            std::vector<u32> primitiveList; // only if leaf
            std::vector<u32> triangleList; // only if leaf
            std::vector<u32> lightList; // only if leaf
            std::vector<u32> blasList; // only if leaf
        };

        Box m_aabb;
        std::vector<std::unique_ptr<Node>> m_nodes;
        std::vector<Triangle> m_triangles;
        std::vector<Material> m_triangleMaterials;
        std::vector<Primitive> m_objects;
        std::vector<Light> m_lights;
        std::vector<std::unique_ptr<BVHBuilder>> m_blas;
        std::vector<BlasInstance> m_blasInstances;
    };
}