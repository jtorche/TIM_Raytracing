#pragma once
#include "timCore/type.h"
#include <vector>
#include <unordered_set>

#include "Shaders/core/primitive_cpp.glsl"
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
        Box aabb;
    };

    enum class CollisionType { Disjoint, Intersect, Contained };

    struct BVHBuildParameters
    {
        // Best parameters for sponza
        u32 maxDepth = 25;
        u32 minObjPerNode = 4;
        u32 minObjGain = 4;
        float expandNodeVolumeThreshold = 0.2f;
        float expandNodeDimensionFactor = 0.225f;
    };

    class BVHBuilder
    {
    public:
        static Material createLambertianMaterial(vec3 _color);
        static Material createPbrMaterial(vec3 _color, float _metalness = 0);
        static Material createEmissiveMaterial(vec3 _color);
        static Material createMirrorMaterial(vec3 _color, float _mirrorness);
        static Material createTransparentMaterial(vec3 _color, float _refractionIndice, float _reflectivity);
        static void setTextureMaterial(Material& _mat, u32 _texture0, u32 _texture1);

        BVHBuilder(const std::string& _name, const BVHGeometry& _geometry, bool _isTlas) : m_name{ _name }, m_isTlas{ _isTlas }, m_geometryBuffer{ _geometry } { m_triangleMaterials.push_back(createLambertianMaterial({ 1,1,1 })); }

        void dumpStats() const;

        void addSphere(const Sphere&, const Material& _mat = createLambertianMaterial({ 0.7f, 0.7f, 0.7f }));
        void addBox(const Box& _box, const Material& _mat = createLambertianMaterial({ 0.7f, 0.7f, 0.7f }));
        void addTriangle(const BVHGeometry::TriangleData& _triangle, const Material& _mat = createLambertianMaterial({ 0.7f, 0.7f, 0.7f }));
        void addTriangleList(u32 _vertexOffset, u32 _numTriangle, const u32 * _indexData, const Material& _mat = createLambertianMaterial({ 0.7f, 0.7f, 0.7f }));
        void addBlas(std::unique_ptr<BVHBuilder> _blas);
        void addSphereLight(const SphereLight& _light);
        void addAreaLight(const AreaLight& _light);
        void mergeBlas(const std::unique_ptr<BVHBuilder>& _blas);

        void buildBlas(const BVHBuildParameters& _params);
        void build(bool _useMultipleThreads);
        void computeSceneAABB();
        void setParameters(const BVHBuildParameters&);

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
        void fillTriangles(byte* _outData, u32 _matIdOffset = 0) const;

    private:
        void addTriangle(const BVHGeometry::TriangleData& _triangle, u32 _materialId);

        struct Node;
        struct SplitData;

        using ObjectIt = std::vector<u32>::iterator;
        void addObjectsRec(u32 _depth, u32 _numUniqueItems,
                           ObjectIt _objectsBegin, ObjectIt _objectsEnd,
                           ObjectIt _trianglesBegin, ObjectIt _trianglesEnd, 
                           ObjectIt _blasBegin, ObjectIt _blasEnd,
                           Node* _curNode, bool _useMultipleThreads);

        template<typename Fun1, typename Fun2>
        void searchBestSplit(Node* _curNode, ObjectIt _objectsBegin, ObjectIt _objectsEnd, ObjectIt _trianglesBegin, ObjectIt _trianglesEnd, ObjectIt _blasBegin, ObjectIt _blasEnd,
                             const Fun1& _movingAxis, const Fun2& _fixedAxis, SplitData& _splitData) const;

        template<bool FillItems>
        void fillSplitData(SplitData& _splitData, const Box& parentBox, const Box& leftBox, const Box& rightBox,
                           ObjectIt _objectsBegin, ObjectIt _objectsEnd, ObjectIt _trianglesBegin, ObjectIt _trianglesEnd, ObjectIt _blasBegin, ObjectIt _blasEnd) const;

        void fillLeafData(Node* _curNode, u32 _depth, ObjectIt _objectsBegin, ObjectIt _objectsEnd, ObjectIt _trianglesBegin, ObjectIt _trianglesEnd, ObjectIt _blasBegin, ObjectIt _blasEnd);
        void fillTriangleStrips(Node* _leaf);
        static float computeAvgObjGain(const SplitData& _data);
        static float computeSplitScore(const SplitData& _data);

        void packNodeData(PackedBVHNode* _outNode, const BVHBuilder::Node& _node, u32 _leafDataOffset);

        Box getAABB(const Triangle& _triangle) const;
        Box getAABB(const Primitive& _prim) const;
        CollisionType triangleBoxCollision(const Triangle& _triangle, const Box& _box) const;
        CollisionType primitiveBoxCollision(const Primitive& _prim, const Box& _box) const;
        CollisionType primitiveSphereCollision(const Primitive& _prim, const Sphere& _sphere) const;

        struct Stats
        {
            u32 numLeafs = 0;
            u32 maxTriangle = 0;
            u32 maxBlas = 0;
            u32 maxDepth = 0;
            float meanTriangle = 0;
            float meanDepth = 0;
            float meanTriangleStrip = 0;
            u32 numDuplicatedTriangle = 0;
            u32 numDuplicatedBlas = 0;
            

            std::unordered_set<u32> m_triangles;
            std::unordered_set<u32> m_allBlas;
            std::unordered_map<u32, u32> m_duplicatedBlas;
        };
        void computeStatsRec(Stats& _stats, Node* _curNode, u32 _depth) const;

    private:
        std::string m_name;
        const u32 m_bufferAlignment = 32;
        BVHBuildParameters m_params;
        bool m_isTlas;
        std::mutex m_mutex;

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

            std::vector<u32> primitiveList;
            std::vector<u32> triangleList;
            std::vector<u32> lightList;
            std::vector<u32> blasList;
            std::vector<TriangleStrip> strips;
        };

        struct SplitData
        {
            u32 numItemsLeft = 0;
            u32 numItemsRight = 0;
            u32 numItemsInBoth = 0;
            u32 numUniqueItemsLeft = 0;
            u32 numUniqueItemsRight = 0;
            Box leftBox;
            Box rightBox;

            std::vector<u32> objectLeft;
            std::vector<u32> objectRight;

            std::vector<u32> triangleLeft;
            std::vector<u32> triangleRight;

            std::vector<u32> blasLeft;
            std::vector<u32> blasRight;
        };

        Box m_aabb;
        float m_meanTriangleSize = 0;
        std::vector<std::unique_ptr<Node>> m_nodes;
        std::vector<Triangle> m_triangles;
        std::vector<Material> m_triangleMaterials;
        std::vector<Primitive> m_objects;
        std::vector<Light> m_lights;
        std::vector<std::unique_ptr<BVHBuilder>> m_blas;
        std::vector<u32> m_blasMaterialIdOffset;
        std::vector<BlasInstance> m_blasInstances;
    };

    struct Edge
    {
        u16 v1, v2;

        Edge() : v1(0xFFFF), v2(0xFFFF) {}
        Edge(u16 _v1, u16 _v2) : v1(_v1 < _v2 ? _v1 : _v2), v2(_v1 < _v2 ? _v2 : _v1) {}
        bool operator<(Edge e) const { return (e.v1 != v1) ? (v1 < e.v1) : (v2 < e.v2); }
        bool operator==(Edge e) const { return v1 == e.v1 && v2 == e.v2; }
    };
}