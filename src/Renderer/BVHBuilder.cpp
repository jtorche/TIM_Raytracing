#include "BVHBuilder.h"
#include "timCore/Common.h"
#include "timCore/flat_hash_map.h"

#include "TriBoxCollision.hpp"
#include <thread>
#include <set>
#include <map>
#include <algorithm>
#include <execution>
#include "shaders/struct_cpp.glsl"

#define INLINE_TRIANGLES 1
#define INLINE_STRIPS    0

template<>
struct ::std::hash<tim::Edge>
{
    std::size_t operator()(tim::Edge e) const noexcept
    {
        return std::hash<u32>{}(u32(e.v1) + (u32(e.v2) << 16));
    }
};

namespace tim
{
    namespace
    {
        constexpr u32 g_MaxMaterialCount = 1u << 16;
        constexpr u32 g_MaxNodeCount = NID_MASK;
        constexpr u32 g_MaxPrimitiveCount = (1u << 16) - 1;

        CollisionType sphereBoxCollision(const Sphere& _sphere, const Box& _box)
        {
            vec3 closestPointInAabb = linalg::min_(linalg::max_(_sphere.center, _box.minExtent), _box.maxExtent);
            float distanceSquared = linalg::length2(closestPointInAabb - _sphere.center);

            return distanceSquared < (_sphere.radius* _sphere.radius) ? CollisionType::Intersect : CollisionType::Disjoint;
        }

        CollisionType boxBoxCollision(const Box& _box1, const Box& _box2)
        {
            if (_box1.minExtent.x >= _box2.maxExtent.x || _box1.minExtent.y >= _box2.maxExtent.y || _box1.minExtent.z >= _box2.maxExtent.z ||
                _box2.minExtent.x >= _box1.maxExtent.x || _box2.minExtent.y >= _box1.maxExtent.y || _box2.minExtent.z >= _box1.maxExtent.z)
            {
                return CollisionType::Disjoint;
            }

            if (_box1.minExtent.x >= _box2.minExtent.x && _box1.minExtent.y >= _box2.minExtent.y && _box1.minExtent.z >= _box2.minExtent.z &&
                _box1.maxExtent.x <= _box2.maxExtent.x && _box1.maxExtent.y <= _box2.maxExtent.y && _box1.maxExtent.z <= _box2.maxExtent.z)
            {
                return CollisionType::Contained;
            }

            return CollisionType::Intersect;
        }

        bool pointBoxCollision(const vec3& _p, const Box& _box)
        {
            return _p.x < _box.minExtent.x || _p.y < _box.minExtent.y || _p.z < _box.minExtent.z ||
                   _p.x > _box.maxExtent.x || _p.y > _box.maxExtent.y || _p.z > _box.maxExtent.z
                   ? false : true;
        }

        CollisionType triangleVerticesBoxCollision(vec3 p0, vec3 p1, vec3 p2, const Box& _box)
        {
            if (pointBoxCollision(p0, _box) || pointBoxCollision(p1, _box), pointBoxCollision(p2, _box))
                return CollisionType::Intersect;
            else
            {
                vec3 boxCenter = (_box.minExtent + _box.maxExtent) * 0.5f;
                vec3 boxHalf = (_box.maxExtent - _box.minExtent) * 0.5f;

                float boxCenter3f[3] = { boxCenter.x, boxCenter.y, boxCenter.z };
                float boxHalf3f[3] = { boxHalf.x, boxHalf.y, boxHalf.z };
                float vec3f3f[3][3] = { { p0.x,p0.y,p0.z }, { p1.x,p1.y,p1.z }, { p2.x,p2.y,p2.z } };
                return triBoxOverlap(boxCenter3f, boxHalf3f, vec3f3f) == 0 ? CollisionType::Disjoint : CollisionType::Intersect;
            }
        }

        float getBoxVolume(const Box& _box)
        {
            vec3 dim = _box.maxExtent - _box.minExtent;
            return dim.x * dim.y * dim.z;
        }

        bool checkBox(const Box& _box)
        {
            return _box.minExtent.x <= _box.maxExtent.x && _box.minExtent.y <= _box.maxExtent.y && _box.minExtent.z <= _box.maxExtent.z;
        }

        Sphere getBoundingSphere(const Light& _light)
        {
            switch (_light.type)
            {
            case Light_Sphere:
                return Sphere{ _light.m_sphere.pos, _light.m_sphere.radius, 1.f / _light.m_sphere.radius };
            case Light_Area:
                return Sphere{ _light.m_area.pos, _light.m_area.attenuationRadius, 1.f / _light.m_area.attenuationRadius };

            default:
                TIM_ASSERT(false);
                return {};
            }
        }
    }

    CollisionType BVHBuilder::triangleBoxCollision(const Triangle& _triangle, const Box& _box) const
    {
        vec3 p0 = m_geometryBuffer.getVertexPosition(_triangle.vertexOffset, _triangle.index01 & 0x0000FFFF);
        vec3 p1 = m_geometryBuffer.getVertexPosition(_triangle.vertexOffset, (_triangle.index01 & 0xFFFF0000) >> 16);
        vec3 p2 = m_geometryBuffer.getVertexPosition(_triangle.vertexOffset, _triangle.index2_matId & 0x0000FFFF);
        return triangleVerticesBoxCollision(p0, p1, p2, _box);
    }

    CollisionType BVHBuilder::primitiveBoxCollision(const Primitive& _prim, const Box& _box) const
    {
        switch (_prim.type)
        {
        case Primitive_Sphere:
            return sphereBoxCollision(_prim.m_sphere, _box);
        case Primitive_AABB:
            return boxBoxCollision(_prim.m_aabb, _box);
        default:
            TIM_ASSERT(false);
            return {};
        }
    }

    CollisionType BVHBuilder::primitiveSphereCollision(const Primitive& _prim, const Sphere& _sphere) const
    {
        switch (_prim.type)
        {
        case Primitive_Sphere:
        {
            float dist2 = _prim.m_sphere.radius + _sphere.radius;
            dist2 *= dist2;
            return linalg::length2(_prim.m_sphere.center - _sphere.center) < dist2 ? CollisionType::Intersect : CollisionType::Disjoint;
        }
        case Primitive_AABB:
            return sphereBoxCollision(_sphere, _prim.m_aabb);
        default:
            TIM_ASSERT(false);
            return {};
        }
    }

    Box BVHBuilder::getAABB(const Triangle& _triangle) const
    {
        vec3 p0 = m_geometryBuffer.getVertexPosition(_triangle.vertexOffset, _triangle.index01 & 0x0000FFFF);
        vec3 p1 = m_geometryBuffer.getVertexPosition(_triangle.vertexOffset, (_triangle.index01 & 0xFFFF0000) >> 16);
        vec3 p2 = m_geometryBuffer.getVertexPosition(_triangle.vertexOffset, _triangle.index2_matId & 0x0000FFFF);
        Box box;
        box.minExtent = { std::min({ p0.x, p1.x, p2.x }), std::min({ p0.y, p1.y, p2.y }), std::min({ p0.z, p1.z, p2.z }) };
        box.maxExtent = { std::max({ p0.x, p1.x, p2.x }), std::max({ p0.y, p1.y, p2.y }), std::max({ p0.z, p1.z, p2.z }) };

        return box;
    }

    Box BVHBuilder::getAABB(const Primitive& _prim) const
    {
        switch (_prim.type)
        {
        case Primitive_Sphere:
        {
            vec3 splatRadius = { _prim.m_sphere.radius, _prim.m_sphere.radius, _prim.m_sphere.radius };
            return Box{ _prim.m_sphere.center - splatRadius, _prim.m_sphere.center + splatRadius };
        }
        case Primitive_AABB:
            return _prim.m_aabb;
        default:
            TIM_ASSERT(false);
            return {};
        }
    }

    Box mergeBox(const Box& _b1, const Box& _b2)
    {
        Box box;
        box.minExtent = linalg::min_(_b1.minExtent, _b2.minExtent);
        box.maxExtent = linalg::max_(_b1.maxExtent, _b2.maxExtent);
        return box;
    }

    Box intersectionBox(const Box& _b1, const Box& _b2)
    {
        Box box;
        box.minExtent = linalg::max_(_b1.minExtent, _b2.minExtent);
        box.maxExtent = linalg::min_(_b1.maxExtent, _b2.maxExtent);
        return box;
    }

    Material BVHBuilder::createLambertianMaterial(vec3 _color)
    {
        Material mat;
        mat.type_ids = { Material_Lambert, u32(-1),u32(-1),u32(-1) };
        mat.color = { _color.x, _color.y, _color.z, 0 };

        return mat;
    }

    Material BVHBuilder::createPbrMaterial(vec3 _color, float _metalness)
    {
        Material mat;
        mat.type_ids = { Material_PBR, u32(-1),u32(-1),u32(-1) };
        mat.color = { _color.x, _color.y, _color.z, 0 };
        mat.params.x = _metalness;
        return mat;
    }

    Material BVHBuilder::createEmissiveMaterial(vec3 _color)
    {
        Material mat;
        mat.type_ids = { Material_Emissive, u32(-1),u32(-1),u32(-1) };
        mat.color = { _color.x, _color.y, _color.z, 0 };

        return mat;
    }

    Material BVHBuilder::createTransparentMaterial(vec3 _color, float _refractionIndice, float _reflectivity)
    {
        Material mat;
        mat.type_ids = { Material_Transparent, u32(-1),u32(-1),u32(-1) };
        mat.color = { _color.x, _color.y, _color.z, 0 };
        mat.params.x = _reflectivity;
        mat.params.y = _refractionIndice;

        return mat;
    }

    void BVHBuilder::setTextureMaterial(Material& _mat, u32 _texture0, u32 _texture1)
    {
        _mat.type_ids.y = _texture0 + (_texture1 << 16);
    }

    void BVHBuilder::addSphere(const Sphere& _sphere, const Material& _mat)
    {
        m_objects.push_back({ _sphere, _mat });
        m_objects.back().m_sphere.invRadius = 1.f / _sphere.radius;
    }

    void BVHBuilder::addBox(const Box& _box, const Material& _mat)
    {
        m_objects.push_back({ _box, _mat });
    }

    void BVHBuilder::addTriangle(const BVHGeometry::TriangleData& _triangle, u32 _materialId)
    {
        TIM_ASSERT(_materialId < g_MaxMaterialCount);
        Triangle triangle;
        triangle.vertexOffset = _triangle.vertexOffset;
        triangle.index01 = _triangle.index[0] + (_triangle.index[1] << 16);
        triangle.index2_matId = _triangle.index[2] + (_materialId << 16);
        m_triangles.push_back(triangle);
    }

    void BVHBuilder::addTriangle(const BVHGeometry::TriangleData& _triangle, const Material& _mat)
    {
        u32 matId = (u32)m_triangleMaterials.size();
        m_triangleMaterials.push_back(_mat);

        addTriangle(_triangle, matId);
    }

    void BVHBuilder::addTriangleList(u32 _vertexOffset, u32 _numTriangle, const u32* _indexData, const Material& _mat)
    {
        u32 matId = (u32)m_triangleMaterials.size();
        m_triangleMaterials.push_back(_mat);

        BVHGeometry::TriangleData triangle;
        triangle.vertexOffset = _vertexOffset;
        for (u32 i = 0; i < _numTriangle; ++i)
        {
            triangle.index[0] = _indexData[i * 3];
            triangle.index[1] = _indexData[i * 3 + 1];
            triangle.index[2] = _indexData[i * 3 + 2];

            addTriangle(triangle, matId);
        }
    }

    void BVHBuilder::addSphereLight(const SphereLight& _light)
    {
        m_lights.push_back({ _light });
    }

    void BVHBuilder::addAreaLight(const AreaLight& _light)
    {
        m_lights.push_back({ _light });
    }

    void BVHBuilder::addBlas(std::unique_ptr<BVHBuilder> _blas)
    {
        u32 matIdOffset = (u32)m_triangleMaterials.size();
        m_blasMaterialIdOffset.push_back(matIdOffset);

        m_triangleMaterials.insert(m_triangleMaterials.end(), _blas->m_triangleMaterials.begin(), _blas->m_triangleMaterials.end());

        m_blasInstances.push_back({ u32(m_blas.size()), {} });
        m_blas.push_back(std::move(_blas));
    }

    void BVHBuilder::mergeBlas(const std::unique_ptr<BVHBuilder>& _blas)
    {
        u32 matOffset = u32(m_triangleMaterials.size());
        for (Triangle tri : _blas->m_triangles)
        {
            tri.index2_matId += (matOffset << 16);
            m_triangles.push_back(tri);
        }

        m_triangleMaterials.insert(m_triangleMaterials.end(), _blas->m_triangleMaterials.begin(), _blas->m_triangleMaterials.end());
    }

    void BVHBuilder::dumpStats() const
    {
        Stats stats;
        computeStatsRec(stats, m_nodes[0].get(), 0);

        stats.meanTriangle /= stats.numLeafs;
        stats.meanTriangleStrip /= stats.numLeafs;
        stats.meanDepth /= stats.numLeafs;

        std::cout << "BVHBuilder stats:\n";
        std::cout << " - Triangles count: " << m_triangles.size() << "\n";
        std::cout << " - leafs count: " << stats.numLeafs << "\n";
        std::cout << " - mean triangles: " << stats.meanTriangle << "\n";
        std::cout << " - mean strips: " << stats.meanTriangleStrip << "\n";
        std::cout << " - mean depth: " << stats.meanDepth << "\n";
        std::cout << " - max triangles: " << stats.maxTriangle << "\n";
        std::cout << " - max blas: " << stats.maxBlas << "\n";
        std::cout << " - max depth: " << stats.maxDepth << "\n";
        std::cout << " - duplicated triangles: " << stats.numDuplicatedTriangle << "\n";
        std::cout << " - duplicated blas: " << stats.numDuplicatedBlas << "\n";

        for (auto [dupBlas, count] : stats.m_duplicatedBlas)
        {
            std::cout << dupBlas << " : " << m_blas[m_blasInstances[dupBlas].blasId]->getTrianglesCount() << " x " << count << std::endl;
        }
    }

    void BVHBuilder::computeStatsRec(Stats& _stats, Node* _curNode, u32 _depth) const
    {
        for (u32 blas : _curNode->blasList)
        {
            auto [_, inserted] = _stats.m_allBlas.insert(blas);
            if (!inserted)
            {
                _stats.numDuplicatedBlas++;
                _stats.m_duplicatedBlas[blas]++;
            }
        }

        for (u32 tri : _curNode->triangleList)
        {
            auto [_, inserted] = _stats.m_triangles.insert(tri);
            if (!inserted)
                _stats.numDuplicatedTriangle++;
        }

        if (_curNode->left && _curNode->right)
        {
            computeStatsRec(_stats, _curNode->left, _depth + 1);
            computeStatsRec(_stats, _curNode->right, _depth + 1);
        }
        else
        {
            _stats.numLeafs++;
            _stats.maxTriangle = std::max(_stats.maxTriangle, u32(_curNode->triangleList.size()));
            _stats.maxBlas = std::max(_stats.maxBlas, u32(_curNode->blasList.size()));
            _stats.maxDepth = std::max(_stats.maxDepth, _depth);
            _stats.meanTriangle += u32(_curNode->triangleList.size());
            _stats.meanTriangleStrip += u32(_curNode->strips.size());
            _stats.meanDepth += _depth;
        }
    }

    void BVHBuilder::setParameters(const BVHBuildParameters& _params)
    {
        m_params = _params;
    }

    void BVHBuilder::buildBlas(const BVHBuildParameters& _params)
    {
#ifdef _DEBUG
        std::for_each(std::execution::seq, m_blas.begin(), m_blas.end(),
#else
        std::for_each(std::execution::par, m_blas.begin(), m_blas.end(),
            // std::for_each(std::execution::seq, m_blas.begin(), m_blas.end(),
#endif
            [&_params](auto& blas)
            {
                blas->setParameters(_params);
                blas->build(true);
                blas->dumpStats();
            });

        // std::for_each(std::execution::seq, m_blas.begin(), m_blas.end(), [](auto& blas) { blas->dumpStats(); });
    }

    static Box adjustAABB(Box _box)
    {
        _box.maxExtent += vec3(float(1e-6));
        _box.minExtent -= vec3(float(1e-6));
        return _box;
    }

    void BVHBuilder::build(bool _useMultipleThreads)
    {
        m_stats = {};

        m_nodes.clear();
        m_nodes.push_back(std::make_unique<Node>(0));

        m_meanTriangleSize = 0;

        // Compute true AABB of the scene
        Box tightBox = { {  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max() },
                         { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() } };
        for (u32 i = 0; i < m_objects.size(); ++i)
        {
            Box box = getAABB(m_objects[i]);
            tightBox.minExtent = linalg::min_(tightBox.minExtent, box.minExtent);
            tightBox.maxExtent = linalg::max_(tightBox.maxExtent, box.maxExtent);
        }
        for (u32 i = 0; i < m_triangles.size(); ++i)
        {
            Box box = getAABB(m_triangles[i]);
            tightBox.minExtent = linalg::min_(tightBox.minExtent, box.minExtent);
            tightBox.maxExtent = linalg::max_(tightBox.maxExtent, box.maxExtent);
            m_meanTriangleSize += linalg::sum(box.maxExtent - box.minExtent) / 3;
        }
        for (u32 i = 0; i < m_blasInstances.size(); ++i)
        {
            m_blasInstances[i].aabb = m_blas[m_blasInstances[i].blasId]->getAABB();
            tightBox.minExtent = linalg::min_(tightBox.minExtent, m_blasInstances[i].aabb.minExtent);
            tightBox.maxExtent = linalg::max_(tightBox.maxExtent, m_blasInstances[i].aabb.maxExtent);
        }
        m_aabb = adjustAABB(tightBox);
        m_nodes.back()->extent = m_aabb;

        m_meanTriangleSize /= m_triangles.size();
        m_meanTriangleSize *= m_params.expandNodeDimensionFactor;

        std::vector<u32> objectsIds(m_objects.size());
        for (u32 i = 0; i < m_objects.size(); ++i)
            objectsIds[i] = i;

        std::vector<u32> triangleIds(m_triangles.size());
        for (u32 i = 0; i < m_triangles.size(); ++i)
            triangleIds[i] = i;

        std::vector<u32> blasIds(m_blasInstances.size());
        for (u32 i = 0; i < m_blasInstances.size(); ++i)
            blasIds[i] = i;

        const u32 numItems = u32(m_objects.size() + m_triangles.size() + m_blasInstances.size());
        addObjectsRec(0, numItems, objectsIds.begin(), objectsIds.end(), triangleIds.begin(), triangleIds.end(), blasIds.begin(), blasIds.end(), m_nodes[0].get(), _useMultipleThreads);

        TIM_ASSERT(m_nodes.size() < g_MaxNodeCount);
    }

    void BVHBuilder::computeSceneAABB()
    {
        Box tightBox = { {  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max() },
                         { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() } };
        for (u32 i = 0; i < m_objects.size(); ++i)
        {
            Box box = getAABB(m_objects[i]);
            tightBox.minExtent = linalg::min_(tightBox.minExtent, box.minExtent);
            tightBox.maxExtent = linalg::max_(tightBox.maxExtent, box.maxExtent);
        }
        for (u32 i = 0; i < m_triangles.size(); ++i)
        {
            Box box = getAABB(m_triangles[i]);
            tightBox.minExtent = linalg::min_(tightBox.minExtent, box.minExtent);
            tightBox.maxExtent = linalg::max_(tightBox.maxExtent, box.maxExtent);
        }
        for (u32 i = 0; i < m_blasInstances.size(); ++i)
        {
            m_blasInstances[i].aabb = m_blas[m_blasInstances[i].blasId]->getAABB();
            tightBox.minExtent = linalg::min_(tightBox.minExtent, m_blasInstances[i].aabb.minExtent);
            tightBox.maxExtent = linalg::max_(tightBox.maxExtent, m_blasInstances[i].aabb.maxExtent);
        }
        m_aabb = adjustAABB(tightBox);
    }

    float BVHBuilder::computeAvgObjGain(const SplitData& _data)
    {
        u32 total = _data.numItemsInBoth + _data.numUniqueItemsLeft + _data.numUniqueItemsRight;

        // A cube has 6 faces, 2 of them will go through both leafs if we face them. So we have 1/3 chance to go through both leafs.
        constexpr float inv3 = 1.f / 3;

        // float avgObjAfterSplit = inv3 * (_numObjLeft + _numObjRight) + 2 * inv3 * (_numObjLeft * _boxVolumeLeft + _numObjRight * _boxVolumeRight) / (_boxVolumeLeft + _boxVolumeRight);
        float boxVolumeLeft = getBoxVolume(_data.leftBox);
        float boxVolumeRight = getBoxVolume(_data.rightBox);
        float avgObjAfterSplit = inv3 * total + 2 * inv3 * (_data.numItemsLeft * boxVolumeLeft + _data.numItemsRight * boxVolumeRight) / (boxVolumeLeft + boxVolumeRight);

        return std::max(0.f, float(total) - avgObjAfterSplit);
    }

    float BVHBuilder::computeSplitScore(const SplitData& _data)
    {
        u32 total = _data.numUniqueItemsLeft + _data.numUniqueItemsRight;
        return float(total - std::max(_data.numUniqueItemsLeft, _data.numUniqueItemsRight));
    }

    namespace TriangleStripHelpers
    {
        u16 getVertex(const Triangle& _tri, u32 _index)
        {
            u16 v[] = { _tri.index01 & 0x0000FFFF, (_tri.index01 & 0xFFFF0000) >> 16, _tri.index2_matId & 0x0000FFFF };
            return v[_index];
        }

        u16 getMaterial(const Triangle& _tri)
        {
            return (_tri.index2_matId & 0xFFFF0000) >> 16;
        }

        bool hasCommonEdge(const Triangle& _tri1, Edge _edge)
        {
            Edge e0 = Edge(getVertex(_tri1, 0), getVertex(_tri1, 1));
            Edge e1 = Edge(getVertex(_tri1, 1), getVertex(_tri1, 2));
            Edge e2 = Edge(getVertex(_tri1, 2), getVertex(_tri1, 0));

            return _edge == e0 || _edge == e1 || _edge == e2;
        }

        bool belongTo(u16 _vertex, const Triangle& _triangle)
        {
            return _vertex == getVertex(_triangle, 0) || _vertex == getVertex(_triangle, 1) || _vertex == getVertex(_triangle, 2);
        }

        u64 triangleVerticesUniqueID(const Triangle& _tri)
        {
            u16 ids[] = { getVertex(_tri, 0), getVertex(_tri, 1), getVertex(_tri, 2) };
            std::sort(std::begin(ids), std::end(ids));
            return u64(ids[0]) + (u64(ids[1]) << 16) + (u64(ids[2]) << 32);
        }

        void removeDuplicates(std::vector<u32>& _triangleIds, const std::vector<Triangle>& _triangles)
        {
            std::sort(_triangleIds.begin(), _triangleIds.end(), [&](u32 tri1, u32 tri2)
                {
                    if (_triangles[tri1].vertexOffset != _triangles[tri2].vertexOffset)
                        return _triangles[tri1].vertexOffset < _triangles[tri2].vertexOffset;
                    else if (getMaterial(_triangles[tri1]) != getMaterial(_triangles[tri2]))
                        return getMaterial(_triangles[tri1]) < getMaterial(_triangles[tri2]);
                    else return triangleVerticesUniqueID(_triangles[tri1]) < triangleVerticesUniqueID(_triangles[tri2]);
                });

            auto last = std::unique(_triangleIds.begin(), _triangleIds.end(), [&](u32 tri1, u32 tri2)
                {
                    return _triangles[tri1].vertexOffset == _triangles[tri2].vertexOffset &&
                           getMaterial(_triangles[tri1]) == getMaterial(_triangles[tri2]) &&
                           triangleVerticesUniqueID(_triangles[tri1]) == triangleVerticesUniqueID(_triangles[tri2]);
                });

            _triangleIds.erase(last, _triangleIds.end());
        }

        TriangleStrip packStrip(const std::vector<u16>& _vertices, u32 _voffset, u16 _matId)
        {
            TIM_ASSERT(_vertices.size() >= 3);
            TIM_ASSERT(_vertices.size() <= 5);

            TriangleStrip strip;
            strip.vertexOffset = _voffset;
            strip.index01 = _vertices[0] | (_vertices[1]) << 16;
            strip.index2_matId = _vertices[2] | (_matId << 16);

            switch (_vertices.size())
            {
            case 3:
                strip.index34 = 0xFFFFffff;
                break;
            case 4:
                strip.index34 = _vertices[3] + 0xFFFF0000;
                break;
            case 5:
                strip.index34 = _vertices[3] | (_vertices[4] << 16);
                break;
            };
            
            return strip;
        }

        void fillTriangleStrips(const std::vector<u32>& _triangleIds, const std::vector<Triangle>& _triangles, std::vector<TriangleStrip>& _strips)
        {
            constexpr u32 MaxTrianglesInStrip = 2;

            ska::flat_hash_set<u32> remainingTriangles(_triangleIds.begin(), _triangleIds.end());
            std::vector<u32> trianglesInStrip;
            std::vector<size_t> uniqueTrianglesInStrip;
            std::vector<u32> remainingTrianglesToRemove;
            std::map<u16, u16> vertexRefCounter;
            std::vector<u16> verticesInStrip;

            while (remainingTriangles.empty() == false)
            {
                trianglesInStrip.clear();
                vertexRefCounter.clear();

                Triangle curTriangle = _triangles[*remainingTriangles.begin()];
                trianglesInStrip.push_back(*remainingTriangles.begin());
                remainingTriangles.erase(remainingTriangles.begin());

                ska::flat_hash_set<Edge> edges;
                edges.insert(Edge(getVertex(curTriangle, 0), getVertex(curTriangle, 1)));
                edges.insert(Edge(getVertex(curTriangle, 1), getVertex(curTriangle, 2)));
                edges.insert(Edge(getVertex(curTriangle, 2), getVertex(curTriangle, 0)));

                for (u32 i : { 0, 1, 2 })
                    vertexRefCounter[getVertex(curTriangle, i)]++;

                bool hasFoundNextStrip;
                u32 stripSize = 1;
                do
                {
                    hasFoundNextStrip = false;
                    remainingTrianglesToRemove.clear();
                    for (u32 tri : remainingTriangles)
                    {
                        if (_triangles[tri].vertexOffset != curTriangle.vertexOffset || getMaterial(_triangles[tri]) != getMaterial(curTriangle))
                            continue;

                        Edge commonEdge;
                        size_t numCommonEdges = std::count_if(std::begin(edges), std::end(edges), [&](Edge _edge)  
                            {
                                bool isCommon = hasCommonEdge(_triangles[tri], _edge);
                                if (isCommon)
                                    commonEdge = _edge;

                                return isCommon;
                            });
                        if (numCommonEdges == 1 && (vertexRefCounter[commonEdge.v1] + vertexRefCounter[commonEdge.v2]) <= 3)
                        {
                            hasFoundNextStrip = true;

                            remainingTrianglesToRemove.push_back(tri);
                            trianglesInStrip.push_back(tri);
                            stripSize++;

                            edges.insert(Edge(getVertex(_triangles[tri], 0), getVertex(_triangles[tri], 1)));
                            edges.insert(Edge(getVertex(_triangles[tri], 1), getVertex(_triangles[tri], 2)));
                            edges.insert(Edge(getVertex(_triangles[tri], 2), getVertex(_triangles[tri], 0)));
                            edges.erase(commonEdge);

                            for (u32 i : { 0, 1, 2 })
                                vertexRefCounter[getVertex(_triangles[tri], i)]++;

                            if (trianglesInStrip.size() == MaxTrianglesInStrip) // we support strip of 5 vertices at most
                                break;
                        }
                    }

                    for (u32 tri : remainingTrianglesToRemove)
                        remainingTriangles.erase(tri);

                } while (hasFoundNextStrip && trianglesInStrip.size() < MaxTrianglesInStrip);

                u16 lastVertex = std::find_if(vertexRefCounter.begin(), vertexRefCounter.end(), [](auto v_count) { return v_count.second == 1; })->first;
                u32 lastTriangle = *std::find_if(trianglesInStrip.begin(), trianglesInStrip.end(), [&](u32 _tri) { return belongTo(lastVertex, _triangles[_tri]); });
                u32 lastTriangleCounter[] = { 
                    vertexRefCounter[getVertex(_triangles[lastTriangle], 0)], 
                    vertexRefCounter[getVertex(_triangles[lastTriangle], 1)], 
                    vertexRefCounter[getVertex(_triangles[lastTriangle], 2)] 
                };

                verticesInStrip.clear();
                while (trianglesInStrip.size() > 1)
                {
                    u16 vertex = std::find_if(vertexRefCounter.begin(), vertexRefCounter.end(), [&](auto v_count) { return v_count.first != lastVertex && v_count.second == 1; })->first;
                    verticesInStrip.push_back(vertex);

                    auto tri = std::find_if(trianglesInStrip.begin(), trianglesInStrip.end(), [&](u32 _tri) { return belongTo(vertex, _triangles[_tri]); });
                    vertexRefCounter[getVertex(_triangles[*tri], 0)]--;
                    vertexRefCounter[getVertex(_triangles[*tri], 1)]--;
                    vertexRefCounter[getVertex(_triangles[*tri], 2)]--;
                    trianglesInStrip.erase(tri);
                }
                
                // Process last triangle
                // 2 special cases, else lastTriangleCounter will always be equivalent to set<u32>{ 1,2,3 }
                if (stripSize == 1)
                {
                    for (u32 i : { 0, 1, 2 })
                        verticesInStrip.push_back(getVertex(_triangles[lastTriangle], i));
                }
                else if (stripSize == 2)
                {
                    for (u32 i : { 0, 1, 2 })
                    {
                        if (lastVertex != getVertex(_triangles[lastTriangle], i))
                            verticesInStrip.push_back(getVertex(_triangles[lastTriangle], i));
                    }
                    verticesInStrip.push_back(lastVertex);
                }
                else // general case
                {
                    TIM_ASSERT(lastTriangleCounter[0] + lastTriangleCounter[1] + lastTriangleCounter[2] == 6);
                    verticesInStrip.push_back(lastTriangleCounter[0] == 3 ? getVertex(_triangles[lastTriangle], 0) : (lastTriangleCounter[1] == 3 ? getVertex(_triangles[lastTriangle], 1) : getVertex(_triangles[lastTriangle], 2)));
                    verticesInStrip.push_back(lastTriangleCounter[0] == 2 ? getVertex(_triangles[lastTriangle], 0) : (lastTriangleCounter[1] == 2 ? getVertex(_triangles[lastTriangle], 1) : getVertex(_triangles[lastTriangle], 2)));
                    verticesInStrip.push_back(lastTriangleCounter[0] == 1 ? getVertex(_triangles[lastTriangle], 0) : (lastTriangleCounter[1] == 1 ? getVertex(_triangles[lastTriangle], 1) : getVertex(_triangles[lastTriangle], 2)));
                }

                _strips.push_back(packStrip(verticesInStrip, curTriangle.vertexOffset, getMaterial(curTriangle)));
            }
        }
    }

    void BVHBuilder::fillTriangleStrips(Node * _leaf)
    {
    #if INLINE_STRIPS
        TriangleStripHelpers::fillTriangleStrips(_leaf->triangleList, m_triangles, _leaf->strips);
        //u32 bestStripsCount = 0xFFFFffff;
        //
        //for (u32 i = 0; i < 64; ++i)
        //{
        //    std::vector<TriangleStrip> strips;
        //    std::next_permutation(_leaf->triangleList.begin(), _leaf->triangleList.end());
        //    TriangleStripHelpers::fillTriangleStrips(_leaf->triangleList, m_triangles, strips);
        //    if (strips.size() < bestStripsCount)
        //    {
        //        bestStripsCount = strips.size();
        //        _leaf->strips = std::move(strips);
        //    }
        //}
    #endif
    }

    void BVHBuilder::fillLeafData(Node* _curNode, u32 _depth, ObjectIt _objectsBegin, ObjectIt _objectsEnd, ObjectIt _trianglesBegin, ObjectIt _trianglesEnd, ObjectIt _blasBegin, ObjectIt _blasEnd)
    {
        const u32 numBlasAndObj = u32(std::distance(_blasBegin, _blasEnd) + std::distance(_objectsBegin, _objectsEnd));

        for (auto it = _objectsBegin; it != _objectsEnd && _curNode->primitiveList.size() < (1u << PrimitiveBitCount); ++it)
            _curNode->primitiveList.push_back(*it);

        for (auto it = _trianglesBegin; it != _trianglesEnd && _curNode->triangleList.size() < (1u << TriangleBitCount); ++it)
            _curNode->triangleList.push_back(*it);

        TriangleStripHelpers::removeDuplicates(_curNode->triangleList, m_triangles);

        for (auto it = _blasBegin; it != _blasEnd && _curNode->blasList.size() < (1u << BlasBitCount); ++it)
            _curNode->blasList.push_back(*it);

        for (u32 i = 0; i < m_lights.size() && _curNode->lightList.size() < (1u << LightBitCount); ++i)
        {
            const Light& light = m_lights[i];
            Sphere sphere = getBoundingSphere(light);

            if (sphereBoxCollision(sphere, _curNode->extent) != CollisionType::Disjoint)
                _curNode->lightList.push_back(u32(i));
        }

        if(_curNode->triangleList.empty() == false)
            fillTriangleStrips(_curNode);
    }

    void BVHBuilder::addObjectsRec(u32 _depth, u32 _numUniqueItems,
                                   ObjectIt _objectsBegin, ObjectIt _objectsEnd, 
                                   ObjectIt _trianglesBegin, ObjectIt _trianglesEnd, 
                                   ObjectIt _blasBegin, ObjectIt _blasEnd, BVHBuilder::Node* _curNode, bool _useMultipleThreads)
    {
        _curNode->primitiveList.clear();
        _curNode->triangleList.clear();
        _curNode->blasList.clear();
        _curNode->lightList.clear();

        u32 numObjects = u32(std::distance(_objectsBegin, _objectsEnd) + std::distance(_trianglesBegin, _trianglesEnd) + std::distance(_blasBegin, _blasEnd));

        // Fill leafs
        if (_numUniqueItems <= m_params.minObjPerNode || _depth >= m_params.maxDepth)
        {
            fillLeafData(_curNode, _depth, _objectsBegin, _objectsEnd, _trianglesBegin, _trianglesEnd, _blasBegin, _blasEnd);
            return;
        }
        else
        {
            constexpr u32 selectBestSplitItemCountThreshold = 1024;
            SplitData bestSplit;
            
            if (numObjects < selectBestSplitItemCountThreshold)
            {
                SplitData splitData[3];
                searchBestSplit(_curNode, _objectsBegin, _objectsEnd, _trianglesBegin, _trianglesEnd, _blasBegin, _blasEnd,
                    [](const vec3& step) { return vec3(step.x, 0, 0); }, [](const vec3& _step) { return vec3{ 0, _step.y, _step.z }; }, splitData[0]);
                searchBestSplit(_curNode, _objectsBegin, _objectsEnd, _trianglesBegin, _trianglesEnd, _blasBegin, _blasEnd,
                    [](const vec3& step) { return vec3(0, step.y, 0); }, [](const vec3& _step) { return vec3{ _step.x, 0, _step.z }; }, splitData[1]);
                searchBestSplit(_curNode, _objectsBegin, _objectsEnd, _trianglesBegin, _trianglesEnd, _blasBegin, _blasEnd,
                    [](const vec3& step) { return vec3(0, 0, step.z); }, [](const vec3& _step) { return vec3{ _step.x, _step.y, 0 }; }, splitData[2]);

                // search for best homogeneous split
                auto getBestSplit = [&](u32 s0, u32 s1) -> u32
                {
                    return computeSplitScore(splitData[s0]) > computeSplitScore(splitData[s1]) ? s0 : s1;
                };


                u32 bestSplitIndex = getBestSplit(getBestSplit(0, 1), 2);
                bestSplit = std::move(splitData[bestSplitIndex]);
            }
            else
            {
                switch (_depth % 3)
                {
                case 0:
                    searchBestSplit(_curNode, _objectsBegin, _objectsEnd, _trianglesBegin, _trianglesEnd, _blasBegin, _blasEnd,
                        [](const vec3& step) { return vec3(step.x, 0, 0); }, [](const vec3& _step) { return vec3{ 0, _step.y, _step.z }; }, bestSplit);
                    break;
                case 1:
                    searchBestSplit(_curNode, _objectsBegin, _objectsEnd, _trianglesBegin, _trianglesEnd, _blasBegin, _blasEnd,
                        [](const vec3& step) { return vec3(0, step.y, 0); }, [](const vec3& _step) { return vec3{ _step.x, 0, _step.z }; }, bestSplit);
                    break;
                case 2:
                    searchBestSplit(_curNode, _objectsBegin, _objectsEnd, _trianglesBegin, _trianglesEnd, _blasBegin, _blasEnd,
                        [](const vec3& step) { return vec3(0, 0, step.z); }, [](const vec3& _step) { return vec3{ _step.x, _step.y, 0 }; }, bestSplit);
                    break;
                }
            }

            //if (bestSplit.numItemsRight == 224 && bestSplit.numItemsLeft == 1)
            //{
            //    vec3 dim = bestSplit.leftBox.maxExtent - bestSplit.leftBox.minExtent;
            //    std::cout << dim.x << " " << dim.y << " " << dim.z << std::endl;
            //    dim = bestSplit.rightBox.maxExtent - bestSplit.rightBox.minExtent;
            //    std::cout << dim.x << " " << dim.y << " " << dim.z << std::endl;
            //    std::cout << "--------------------------\n";
            //
            //    __debugbreak();
            //}

            if (_depth > 0)
            {
                if (numObjects - bestSplit.numItemsLeft < m_params.minObjGain && numObjects - bestSplit.numItemsRight < m_params.minObjGain ||
                    bestSplit.numUniqueItemsLeft == 0 || bestSplit.numUniqueItemsRight == 0)
                {
                    fillLeafData(_curNode, _depth, _objectsBegin, _objectsEnd, _trianglesBegin, _trianglesEnd, _blasBegin, _blasEnd);
                    return;
                }
            }

            // Fill leaf data only for lights
            fillLeafData(_curNode, _depth, _objectsEnd, _objectsEnd, _trianglesEnd, _trianglesEnd, _blasEnd, _blasEnd);

            SplitData bestSplitData;
            fillSplitData<true>(bestSplitData, _curNode->extent, bestSplit.leftBox, bestSplit.rightBox, _objectsBegin, _objectsEnd, _trianglesBegin, _trianglesEnd, _blasBegin, _blasEnd);

            m_mutex.lock();
            m_nodes.push_back(std::make_unique<Node>(m_nodes.size()));
            Node& leftNode = *m_nodes.back();

            m_nodes.push_back(std::make_unique<Node>(m_nodes.size()));
            Node& rightNode = *m_nodes.back();
            m_mutex.unlock();

            leftNode.extent = adjustAABB(bestSplitData.leftBox);
            leftNode.parent = _curNode;
            _curNode->left = &leftNode;

            rightNode.extent = adjustAABB(bestSplitData.rightBox);
            rightNode.parent = _curNode;
            _curNode->right = &rightNode;

            leftNode.sibling = &rightNode;
            rightNode.sibling = &leftNode;

            auto& objectLeft = bestSplitData.objectLeft;
            auto& objectRight = bestSplitData.objectRight;

            auto& triangleLeft = bestSplitData.triangleLeft;
            auto& triangleRight = bestSplitData.triangleRight;

            auto& blasLeft = bestSplitData.blasLeft;
            auto& blasRight = bestSplitData.blasRight;

        #ifndef _DEBUG
            if (_useMultipleThreads && numObjects > 10'000)
            {
                std::thread th1([&]()
                {
                    addObjectsRec(_depth + 1, bestSplitData.numUniqueItemsLeft,
                                  objectLeft.begin(), objectLeft.end(), triangleLeft.begin(), triangleLeft.end(), blasLeft.begin(), blasLeft.end(), &leftNode, _useMultipleThreads);
                });
                std::thread th2([&]()
                {
                    addObjectsRec(_depth + 1, bestSplitData.numUniqueItemsRight,
                                  objectRight.begin(), objectRight.end(), triangleRight.begin(), triangleRight.end(), blasRight.begin(), blasRight.end(), &rightNode, _useMultipleThreads);
                });
                th1.join();
                th2.join();
            }
            else
        #endif
            {
                addObjectsRec(_depth + 1, bestSplitData.numUniqueItemsLeft,
                              objectLeft.begin(), objectLeft.end(), triangleLeft.begin(), triangleLeft.end(), blasLeft.begin(), blasLeft.end(), &leftNode, false);

                addObjectsRec(_depth + 1, bestSplitData.numUniqueItemsRight,
                              objectRight.begin(), objectRight.end(), triangleRight.begin(), triangleRight.end(), blasRight.begin(), blasRight.end(), &rightNode, false);
            }

            if (m_isTlas)
            {
                auto mergeCommonItems = [](std::vector<u32>& _left, std::vector<u32>& _right)
                {
                    std::sort(_left.begin(), _left.end());
                    std::sort(_right.begin(), _right.end());
                    std::vector<u32> commonItems(std::min(_right.size(), _left.size()));
                    auto it = std::set_intersection(_left.begin(), _left.end(), _right.begin(), _right.end(), commonItems.begin());

                    commonItems.resize(std::distance(commonItems.begin(), it));
                    std::vector<u32> newLeft; newLeft.reserve(_left.size() - commonItems.size());
                    std::vector<u32> newRight; newRight.reserve(_right.size() - commonItems.size());

                    auto itLeft = _left.begin();
                    auto itRight = _right.begin();
                    for (u32 i : commonItems)
                    {
                        while (*itLeft != i) newLeft.push_back(*(itLeft++));
                        while (*itRight != i) newRight.push_back(*(itRight++));

                        // skip common item
                        ++itLeft;
                        ++itRight;
                    }

                    while (itLeft != _left.end()) newLeft.push_back(*(itLeft++));
                    while (itRight != _right.end()) newRight.push_back(*(itRight++));

                    _left = std::move(newLeft);
                    _right = std::move(newRight);
                    return commonItems;
                };

                _curNode->blasList = std::move(mergeCommonItems(leftNode.blasList, rightNode.blasList));
                _curNode->primitiveList = std::move(mergeCommonItems(leftNode.primitiveList, rightNode.primitiveList));
            }
        }
    }

    static float computeMaxDistanceBetweenBoxDimensions(const Box& _box1, const Box& _box2)
    {
        vec3 dim1 = _box1.maxExtent - _box1.minExtent;
        vec3 dim2 = _box2.maxExtent - _box2.minExtent;

        return linalg::maxelem(linalg::abs(dim1 - dim2));
    }

    template<bool FillItems>
    void BVHBuilder::fillSplitData(SplitData& _splitData, const Box& parentBox, const Box& leftBox, const Box& rightBox,
                                   ObjectIt _objectsBegin, ObjectIt _objectsEnd, ObjectIt _trianglesBegin, ObjectIt _trianglesEnd, ObjectIt _blasBegin, ObjectIt _blasEnd) const
    {
        _splitData.leftBox = leftBox;
        _splitData.rightBox = rightBox;

        const float leftVolume = getBoxVolume(leftBox);
        const float rightVolume = getBoxVolume(rightBox);

        // This vars are only used if FillItems
        bool isLeftBoxInitialized = false;
        bool isRightBoxInitialized = false;
        Box curLeftBox = leftBox, curRightBox = rightBox;

        auto processObject = [&](auto _fillItems, std::vector<u32>& _itemsLeft, std::vector<u32>& _itemsRight, 
                                 ObjectIt _begin, ObjectIt _end, auto&& _collideObj, auto&& _getAABB)
        {
            for (ObjectIt it = _begin; it != _end; ++it)
            {
                bool collideLeft = _collideObj(it, leftBox) != CollisionType::Disjoint;
                bool collideRight = _collideObj(it, rightBox) != CollisionType::Disjoint;

                Box aabb = _getAABB(it);

                if (!collideLeft && collideRight)
                {
                    _splitData.numItemsRight++;
                    _splitData.numUniqueItemsRight++;
                    
                    if constexpr (_fillItems)
                    {
                        _itemsRight.push_back(*it);
                        _splitData.rightBox = intersectionBox(isRightBoxInitialized ? mergeBox(aabb, _splitData.rightBox) : aabb, curRightBox);
                        isRightBoxInitialized = true;
                    }
                }
                else if (collideLeft && !collideRight)
                {
                    _splitData.numItemsLeft++;
                    _splitData.numUniqueItemsLeft++;
                    
                    if constexpr (_fillItems)
                    {
                        _itemsLeft.push_back(*it);
                        _splitData.leftBox = intersectionBox(isLeftBoxInitialized ? mergeBox(aabb, _splitData.leftBox) : aabb, curLeftBox);
                        isLeftBoxInitialized = true;
                    }
                }
                else if(!collideLeft && !collideRight)
                {
                    // error case, usually due to floating point precision
                    // __debugbreak();  
                }
                else // the item is in both left and right AABBs
                {
                    auto addInBothNodes = [&]()
                    {
                        _splitData.numItemsInBoth++;
                        _splitData.numItemsLeft++;
                        _splitData.numItemsRight++;

                        if constexpr (_fillItems)
                        {
                            _itemsLeft.push_back(*it);
                            _itemsRight.push_back(*it);

                            _splitData.leftBox = intersectionBox(isLeftBoxInitialized ? mergeBox(aabb, _splitData.leftBox) : aabb, curLeftBox);
                            isLeftBoxInitialized = true;

                            _splitData.rightBox = intersectionBox(isRightBoxInitialized ? mergeBox(aabb, _splitData.rightBox) : aabb, curRightBox);
                            isRightBoxInitialized = true;
                        }
                    };

                    float smallestVolume = std::min(leftVolume, rightVolume);
                    Box newLeftBox = intersectionBox(mergeBox(leftBox, aabb), parentBox);
                    Box newRightBox = intersectionBox(mergeBox(rightBox, aabb), parentBox);
                    float diffLeft = getBoxVolume(newLeftBox) - leftVolume;
                    float diffRight = getBoxVolume(newRightBox) - rightVolume;

                    if (diffLeft < diffRight) // better to add in left
                    {
                        float distAugmentation = computeMaxDistanceBetweenBoxDimensions(newLeftBox, leftBox);

                        if ((diffLeft / smallestVolume) >= m_params.expandNodeVolumeThreshold || distAugmentation > m_meanTriangleSize)
                        {
                            // big items added in both side to avoid increasing the size of the AABB to much
                            addInBothNodes();
                        }
                        else
                        {
                            _splitData.numItemsLeft++;
                            _splitData.numUniqueItemsLeft++;

                            if constexpr (_fillItems)
                            {
                                _itemsLeft.push_back(*it);
                                _splitData.leftBox = intersectionBox(isLeftBoxInitialized ? mergeBox(_splitData.leftBox, aabb) : aabb, parentBox);
                                curLeftBox = intersectionBox(mergeBox(aabb, curLeftBox), parentBox);
                                isLeftBoxInitialized = true;
                            }
                        }
                    }
                    else // better to add in right
                    {
                        float distAugmentation = computeMaxDistanceBetweenBoxDimensions(newRightBox, rightBox);

                        if ((diffRight / smallestVolume) >= m_params.expandNodeVolumeThreshold || distAugmentation > m_meanTriangleSize)
                        {
                            // big items added in both side to avoid increasing the size of the AABB to much
                            addInBothNodes();
                        }
                        else
                        {
                            _splitData.numItemsRight++;
                            _splitData.numUniqueItemsRight++;

                            if constexpr (_fillItems)
                            {
                                _itemsRight.push_back(*it);
                                _splitData.rightBox = intersectionBox(isRightBoxInitialized ? mergeBox(_splitData.rightBox, aabb) : aabb, parentBox);
                                curRightBox = intersectionBox(mergeBox(aabb, curRightBox), parentBox);
                                isRightBoxInitialized = true;
                            }
                        }
                    }
                }
            }
        };

        processObject(std::integral_constant<bool, FillItems>{}, _splitData.objectLeft, _splitData.objectRight, _objectsBegin, _objectsEnd,
            [&](ObjectIt it, const Box& _box) { return primitiveBoxCollision(m_objects[*it], _box); },
            [&](ObjectIt it) { return getAABB(m_objects[*it]); });

        processObject(std::integral_constant<bool, FillItems>{}, _splitData.triangleLeft, _splitData.triangleRight, _trianglesBegin, _trianglesEnd,
            [&](ObjectIt it, const Box& _box) { return triangleBoxCollision(m_triangles[*it], _box); },
            [&](ObjectIt it) { return getAABB(m_triangles[*it]); });

        processObject(std::integral_constant<bool, FillItems>{}, _splitData.blasLeft, _splitData.blasRight, _blasBegin, _blasEnd,
            [&](ObjectIt it, const Box& _box) { return boxBoxCollision(m_blasInstances[*it].aabb, _box); },
            [&](ObjectIt it) { return m_blasInstances[*it].aabb; });
    }

    template<typename Fun1, typename Fun2>
    void BVHBuilder::searchBestSplit(BVHBuilder::Node* _curNode,
                                     ObjectIt _objectsBegin, ObjectIt _objectsEnd, ObjectIt _trianglesBegin, ObjectIt _trianglesEnd, ObjectIt _blasBegin, ObjectIt _blasEnd,
                                     const Fun1& _movingAxis, const Fun2& _fixedAxis, SplitData& _splitData) const
    {
        u32 numObjects = u32(std::distance(_objectsBegin, _objectsEnd) + std::distance(_trianglesBegin, _trianglesEnd) + std::distance(_blasBegin, _blasEnd));
        vec3 boxDim = _curNode->extent.maxExtent - _curNode->extent.minExtent;

        if (numObjects > 256)
        {
            vec2 searchBestCut = { 0, 1 };
            for (u32 i = 0; i < 32; ++i)
            {
                float newCut = (searchBestCut.x + searchBestCut.y) * 0.5f;
                vec3 step = _movingAxis(boxDim * newCut);
                vec3 fixedStep = _fixedAxis(boxDim);
        
                Box leftBox = { _curNode->extent.minExtent, _curNode->extent.minExtent + fixedStep + step };
                Box rightBox = { _curNode->extent.minExtent + step, _curNode->extent.maxExtent };
        
                _splitData = {};
                fillSplitData<false>(_splitData, _curNode->extent, leftBox, rightBox, _objectsBegin, _objectsEnd, _trianglesBegin, _trianglesEnd, _blasBegin, _blasEnd);
        
                u32 absDiff = _splitData.numItemsLeft < _splitData.numItemsRight ? _splitData.numItemsRight - _splitData.numItemsLeft : _splitData.numItemsLeft - _splitData.numItemsRight;
                if (absDiff <= std::max<u32>(numObjects / (16*1024), 1)) // best cut have been found
                    break;
        
                if (_splitData.numItemsLeft > _splitData.numItemsRight)
                    searchBestCut.y = newCut;
                else
                    searchBestCut.x = newCut;
            }
        }
        else
        {
            vec3 fixedStep = _fixedAxis(boxDim);
            float score = -1;
            auto processStep = [&](vec3 step)
            {
                Box leftBox = { _curNode->extent.minExtent, _curNode->extent.minExtent + fixedStep + step };
                Box rightBox = { _curNode->extent.minExtent + step, _curNode->extent.maxExtent };

                if (checkBox(leftBox) && checkBox(rightBox))
                {
                    SplitData splitData;
                    fillSplitData<false>(splitData, _curNode->extent, leftBox, rightBox, _objectsBegin, _objectsEnd, _trianglesBegin, _trianglesEnd, _blasBegin, _blasEnd);

                    float currentScore = computeSplitScore(splitData);
                    if (currentScore > score)
                    {
                        score = currentScore;
                        _splitData = std::move(splitData);
                    }
                }
            };

            processStep(_movingAxis(boxDim * 0.5f));

            auto convertToStep = [&](vec3 _absPoint) -> vec3
            {
                vec3 step = _absPoint - _curNode->extent.minExtent;
                return _movingAxis(step);
            };

            const vec3 delta = vec3(float(1e-5), float(1e-5), float(1e-5));
            std::for_each(_blasBegin, _blasEnd, [&](u32 _id)
            {
                const u32 blasId = m_blasInstances[_id].blasId;
                Box box = m_blas[blasId]->m_aabb;
                processStep(convertToStep(box.minExtent - delta));
                processStep(convertToStep(box.maxExtent + delta));
            });

            std::for_each(_objectsBegin, _objectsEnd, [&](u32 _id)
            {
                Box box = getAABB(m_objects[_id]);
                processStep(convertToStep(box.minExtent - delta));
                processStep(convertToStep(box.maxExtent + delta));
            });

            std::for_each(_trianglesBegin, _trianglesEnd, [&](u32 _id)
            {
                Box box = getAABB(m_triangles[_id]);
                processStep(convertToStep(box.minExtent - delta));
                processStep(convertToStep(box.maxExtent + delta));
            });
        }
    }

    u32 BVHBuilder::getBvhGpuSize() const
    {
        u32 size = alignUp<u32>((u32)(m_triangleMaterials.size() + m_objects.size()) * sizeof(Material), m_bufferAlignment);

        u32 numberOfTriangles = (u32)m_triangles.size();
        for (u32 i = 0; i < m_blas.size(); ++i)
            numberOfTriangles += m_blas[i]->getTrianglesCount();

        size += alignUp<u32>(numberOfTriangles * sizeof(Triangle), m_bufferAlignment);
        size += alignUp<u32>((u32)m_objects.size() * sizeof(PackedPrimitive), m_bufferAlignment);
        size += alignUp<u32>((u32)m_lights.size() * sizeof(PackedLight), m_bufferAlignment);
        size += alignUp<u32>((u32)m_blasInstances.size() * sizeof(BlasHeader), m_bufferAlignment);

        auto sizeOfNodes = [this](const auto& _nodes)
        {
            u32 size = 0;
            size += alignUp<u32>((u32)_nodes.size() * sizeof(PackedBVHNode), m_bufferAlignment);
            for (const auto& n : _nodes)
            {
                size += (u32)(1 + n->primitiveList.size() + n->lightList.size() + n->blasList.size()) * sizeof(u32);
            #if INLINE_TRIANGLES
                size += u32(n->triangleList.size()) * sizeof(Triangle);
            #elif INLINE_STRIPS
                size += u32(n->strips.size()) * sizeof(TriangleStrip);
            #else  
                size += u32(n->triangleList.size()) * sizeof(u32);
            #endif      
            }

            return size;
        };
        size += sizeOfNodes(m_nodes);

        for (const auto& blas : m_blas)
            size += sizeOfNodes(blas->m_nodes);

        return size;
    }

    namespace
    {
        void packSphere(PackedPrimitive* prim, const Sphere& _sphere)
        {
            prim->fparam[0] = _sphere.center.x;
            prim->fparam[1] = _sphere.center.y;
            prim->fparam[2] = _sphere.center.z;
            prim->fparam[3] = _sphere.radius;
            prim->fparam[4] = _sphere.invRadius;
            prim->fparam[5] = 0;
        }

        void packAabb(PackedPrimitive* prim, const Box& _box)
        {
            prim->fparam[0] = _box.minExtent.x;
            prim->fparam[1] = _box.minExtent.y;
            prim->fparam[2] = _box.minExtent.z;
            prim->fparam[3] = _box.maxExtent.x;
            prim->fparam[4] = _box.maxExtent.y;
            prim->fparam[5] = _box.maxExtent.z;
        }

        void packSphereLight(PackedLight* light, const SphereLight& _pl)
        {
            light->iparam = Light_Sphere;
            light->fparam[0] = _pl.pos.x;
            light->fparam[1] = _pl.pos.y;
            light->fparam[2] = _pl.pos.z;
            light->fparam[3] = _pl.radius;

            light->fparam[4] = _pl.color.x;
            light->fparam[5] = _pl.color.y;
            light->fparam[6] = _pl.color.z;
            light->fparam[7] = _pl.sphereRadius;
        }

        void packAreaLight(PackedLight* light, const AreaLight& _al)
        {
            light->iparam = Light_Area;
            light->fparam[0] = _al.pos.x;
            light->fparam[1] = _al.pos.y;
            light->fparam[2] = _al.pos.z;

            light->fparam[3] = _al.width.x;
            light->fparam[4] = _al.width.y;
            light->fparam[5] = _al.width.z;

            light->fparam[6] = _al.height.x;
            light->fparam[7] = _al.height.y;
            light->fparam[8] = _al.height.z;

            light->fparam[9] = _al.color.x;
            light->fparam[10] = _al.color.y;
            light->fparam[11] = _al.color.z;

            light->fparam[12] = _al.attenuationRadius;
        }
    }

    void BVHBuilder::packNodeData(PackedBVHNode* _outNode, const BVHBuilder::Node& _node, u32 _leafDataOffset)
    {
        TIM_ASSERT((_node.left && _node.right) || (!_node.left && !_node.right));
        constexpr u32 InvalidNodeId = NID_MASK;
        u32 leftIndex = _node.left ? _node.left->nid : InvalidNodeId;
        u32 rightIndex = _node.right ? _node.right->nid : InvalidNodeId;
        u32 parentIndex = _node.parent ? _node.parent->nid : InvalidNodeId;
        u32 siblingIndex = _node.sibling ? _node.sibling->nid : InvalidNodeId;
        
        TIM_ASSERT(leftIndex + 1 == rightIndex || (leftIndex == InvalidNodeId && rightIndex == InvalidNodeId));

        auto isLeaf = [](const BVHBuilder::Node* node) { return node ? node->left == nullptr : false; };
        auto hasParent = [](const BVHBuilder::Node* node) { return node ? node->parent != nullptr : false; };

        // add bit to fast check if leaf
        siblingIndex = isLeaf(_node.sibling) ? NID_LEAF_BIT | siblingIndex : siblingIndex;

        u32 packedChildNid = leftIndex | (isLeaf(_node.left) ? NID_LEFT_LEAF_BIT : 0) | (isLeaf(_node.right) ? NID_RIGHT_LEAF_BIT : 0);
        _outNode->nid = uvec4(parentIndex, siblingIndex, packedChildNid, _leafDataOffset);

        if (_node.left)
        {
            Box box0 = _node.left->extent;
            Box box1 = _node.right->extent;
            _outNode->n0xy = vec4(box0.minExtent.x, box0.minExtent.y, box0.maxExtent.x, box0.maxExtent.y);
            _outNode->n1xy = vec4(box1.minExtent.x, box1.minExtent.y, box1.maxExtent.x, box1.maxExtent.y);
            _outNode->nz = vec4(box0.minExtent.z, box0.maxExtent.z, box1.minExtent.z, box1.maxExtent.z);
        }
        else
        {
            _outNode->n0xy = vec4(0, 0, 0, 0);
            _outNode->n1xy = vec4(0, 0, 0, 0);
            _outNode->nz = vec4(0, 0, 0, 0);
        }
    }

    void BVHBuilder::fillGpuBuffer(void* _data, uvec2& _triangleOffsetRange, uvec2& _primitiveOffsetRange, 
                                   uvec2& _materialOffsetRange, uvec2& _lightOffsetRange, uvec2& _nodeOffsetRange, uvec2& _leafDataOffsetRange, uvec2& _blasOffsetRange)
    {
        ubyte* out = (ubyte*)_data;
        ubyte* prevout = out;

        // Save blas header data write pointer
        ubyte* blasHeaderWritePtr = out;
        u32 blasHeaderBufferSize = u32(m_blasInstances.size() * sizeof(BlasHeader));
        _blasOffsetRange = { (u32)std::distance((ubyte*)_data, out), blasHeaderBufferSize };
        _blasOffsetRange.y = std::max(m_bufferAlignment, _blasOffsetRange.y);

        out = prevout + alignUp(blasHeaderBufferSize, m_bufferAlignment);
        prevout = out;

        // Store triangle data
        u32 totalTriangleCount = u32(m_triangles.size());
        std::vector<u32> blasTriangleOffset(m_blas.size());
        for (u32 i = 0; i < m_blas.size(); ++i)
        {
            blasTriangleOffset[i] = totalTriangleCount;
            totalTriangleCount += m_blas[i]->getTrianglesCount();
        }

        u32 triangleBufferSize = u32(totalTriangleCount * sizeof(Triangle));
        _triangleOffsetRange = { (u32)std::distance((ubyte*)_data, out), triangleBufferSize };
        _triangleOffsetRange.y = std::max(m_bufferAlignment, _triangleOffsetRange.y);
        
        fillTriangles(out);
        out += sizeof(Triangle) * getTrianglesCount();

        for (u32 i = 0; i < m_blas.size(); ++i)
        {
            m_blas[i]->fillTriangles(out, m_blasMaterialIdOffset[i]);
            out += sizeof(Triangle) * m_blas[i]->getTrianglesCount();
        }
        
        out = prevout + alignUp(triangleBufferSize, m_bufferAlignment);
        prevout = out;

        // Store primitive data
        u32 curMaterialId = (u32)m_triangleMaterials.size(); // primitive materials are packed after triangle materials
        u32 objectBufferSize = u32(m_objects.size() * sizeof(PackedPrimitive));
        _primitiveOffsetRange = { (u32)std::distance((ubyte*)_data, out), objectBufferSize };
        _primitiveOffsetRange.y = std::max(m_bufferAlignment, _primitiveOffsetRange.y);

        for (u32 i = 0; i < m_objects.size(); ++i)
        {
            PackedPrimitive* prim = reinterpret_cast<PackedPrimitive*>(out);
            prim->iparam = m_objects[i].type + (curMaterialId << 16);
            switch (m_objects[i].type)
            {
            case Primitive_Sphere:
                packSphere(prim, m_objects[i].m_sphere);
                break;
            case Primitive_AABB:
                packAabb(prim, m_objects[i].m_aabb);
                break;
            default:
            case Primitive_OBB:
                TIM_ASSERT(false);
                break;
            }

            out += sizeof(PackedPrimitive);
            curMaterialId++;
        }
        out = prevout + alignUp(objectBufferSize, m_bufferAlignment);
        prevout = out;

        // Store material data
        u32 materialCount = u32(m_triangleMaterials.size() + m_objects.size());
        TIM_ASSERT(materialCount < g_MaxMaterialCount);
        u32 materialBufferSize = materialCount * sizeof(Material);
        _materialOffsetRange = { (u32)std::distance((ubyte*)_data, out), materialBufferSize };
        _materialOffsetRange.y = std::max(m_bufferAlignment, _materialOffsetRange.y);

        // first pack triangle materials
        for (u32 i = 0; i < m_triangleMaterials.size(); ++i)
        {
            *reinterpret_cast<Material*>(out) = m_triangleMaterials[i];
            out += sizeof(Material);
        }
        for (u32 i = 0; i < m_objects.size(); ++i)
        {
            *reinterpret_cast<Material*>(out) = m_objects[i].material;
            out += sizeof(Material);
        }
        out = prevout + alignUp(materialBufferSize, m_bufferAlignment);
        prevout = out;

        // Store light data
        u32 lightBufferSize = u32(m_lights.size() * sizeof(PackedLight));
        _lightOffsetRange = { (u32)std::distance((ubyte*)_data, out), lightBufferSize };
        _lightOffsetRange.y = std::max(m_bufferAlignment, _lightOffsetRange.y);

        for (u32 i = 0; i < m_lights.size(); ++i)
        {
            PackedLight* lightDat = reinterpret_cast<PackedLight*>(out);
            switch (m_lights[i].type)
            {
            case Light_Sphere:
                packSphereLight(lightDat, m_lights[i].m_sphere);
                break;
            case Light_Area:
                packAreaLight(lightDat, m_lights[i].m_area);
                break;
            default:
                TIM_ASSERT(false);
                break;
            }

            out += sizeof(PackedLight);
        }
        out = prevout + alignUp(lightBufferSize, m_bufferAlignment);
        prevout = out;

        // Store node data
        std::vector<Node*> nodes;
        for (auto& n : m_nodes)
            nodes.push_back(n.get());

        std::vector<u32> blasRootIndex(m_blas.size());

        // merge blas nodes
        for (u32 i = 0; i < m_blas.size(); ++i)
        {
            blasRootIndex[i] = u32(nodes.size());
            for (auto& n : m_blas[i]->m_nodes)
            {
                n->nid += blasRootIndex[i];
                TIM_ASSERT(n->lightList.empty() && n->blasList.empty() && n->primitiveList.empty());

                for (u32& triangleIndex : n->triangleList)
                    triangleIndex += blasTriangleOffset[i];

                nodes.push_back(n.get());
            }
        }

        u32 nodeBufferSize = u32(nodes.size() * sizeof(PackedBVHNode));
        _nodeOffsetRange = { (u32)std::distance((ubyte*)_data, out), nodeBufferSize };
        _nodeOffsetRange.y = std::max(m_bufferAlignment, _nodeOffsetRange.y);

        u32* objectListBegin = reinterpret_cast<u32*>(out + alignUp(nodeBufferSize, m_bufferAlignment));
        u32* objectListCurPtr = objectListBegin;

        for (const BVHBuilder::Node* n : nodes)
        {
            u32 leafDataIndex = u32(objectListCurPtr - objectListBegin);

        #if INLINE_STRIPS
            u32 triangleCount = u32(n->strips.size());
        #else
            u32 triangleCount = u32(n->triangleList.size());
        #endif  

            // First write node data (objects, triangles and lights)
            static_assert(TriangleBitCount + PrimitiveBitCount + LightBitCount + BlasBitCount == 32);
            TIM_ASSERT(triangleCount < (1 << TriangleBitCount));
            TIM_ASSERT(u32(n->blasList.size()) < (1 << BlasBitCount));
            TIM_ASSERT(u32(n->primitiveList.size()) < (1 << PrimitiveBitCount));
            TIM_ASSERT(u32(n->lightList.size()) < (1 << LightBitCount));

            u32 packedCount = triangleCount +
                (u32(n->blasList.size()) << TriangleBitCount) +
                (u32(n->primitiveList.size()) << (TriangleBitCount + BlasBitCount)) +
                (u32(n->lightList.size()) << (TriangleBitCount + BlasBitCount + PrimitiveBitCount));

            *objectListCurPtr = packedCount;
            ++objectListCurPtr;
        #if INLINE_TRIANGLES
            TIM_ASSERT(!m_isTlas);
            for (u32 tri : n->triangleList)
            {
                memcpy(objectListCurPtr, &m_triangles[tri], sizeof(Triangle));
                objectListCurPtr += (sizeof(Triangle) / sizeof(u32));
            }
        #elif INLINE_STRIPS
            TIM_ASSERT(!m_isTlas);
            memcpy(objectListCurPtr, n->strips.data(), n->strips.size() * sizeof(TriangleStrip));
            objectListCurPtr += n->strips.size() * (sizeof(TriangleStrip) / sizeof(u32));
        #else
            memcpy(objectListCurPtr, n->triangleList.data(), sizeof(u32) * n->triangleList.size());
            objectListCurPtr += n->triangleList.size();
        #endif

            memcpy(objectListCurPtr, n->blasList.data(), sizeof(u32) * n->blasList.size());
            objectListCurPtr += n->blasList.size();
            memcpy(objectListCurPtr, n->primitiveList.data(), sizeof(u32) * n->primitiveList.size());
            objectListCurPtr += n->primitiveList.size();
            memcpy(objectListCurPtr, n->lightList.data(), sizeof(u32) * n->lightList.size());
            objectListCurPtr += n->lightList.size();

            // Then write the BVH node itself
            PackedBVHNode* node = reinterpret_cast<PackedBVHNode*>(out);
            packNodeData(node, *n, packedCount != 0 ? leafDataIndex : 0xFFFFffff);
            out += sizeof(PackedBVHNode);
        }

        _leafDataOffsetRange = { (u32)std::distance((ubyte*)_data, (ubyte*)objectListBegin), (u32)std::distance((ubyte*)objectListBegin, (ubyte*)objectListCurPtr) };

        // Fill blas header data
        for (u32 i = 0; i < m_blasInstances.size(); ++i)
        {
            const u32 blasId = m_blasInstances[i].blasId;

            BlasHeader* header = reinterpret_cast<BlasHeader*>(blasHeaderWritePtr);

            header->minExtent = m_blasInstances[i].aabb.minExtent;
            header->maxExtent = m_blasInstances[i].aabb.maxExtent;
            header->rootIndex = blasRootIndex[blasId];

            blasHeaderWritePtr += sizeof(BlasHeader);
        }
    }

    void BVHBuilder::fillTriangles(byte* _outData, u32 _matIdOffset) const
    {
        for (u32 i = 0; i < m_triangles.size(); ++i)
        {
            Triangle* tri = reinterpret_cast<Triangle*>(_outData);
            *tri = m_triangles[i];
            tri->index2_matId += (_matIdOffset << 16);

            _outData += sizeof(Triangle);
        }
    }
}

