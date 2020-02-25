#include "BVHBuilder.h"
#include "timCore/Common.h"

#include "TriBoxCollision.hpp"
#include <thread>
#include "shaders/struct_cpp.glsl"

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

            return distanceSquared < (_sphere.radius * _sphere.radius) ? CollisionType::Intersect : CollisionType::Disjoint;
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
            if(pointBoxCollision(p0, _box) || pointBoxCollision(p1, _box), pointBoxCollision(p2, _box))
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

    Material BVHBuilder::createLambertianMaterial(vec3 _color)
    {
        Material mat;
        mat.type_ids = { Material_Lambert,0,0,0 };
        mat.color = { _color.x, _color.y, _color.z, 0 };

        return mat;
    }

    Material BVHBuilder::createEmissiveMaterial(vec3 _color)
    {
        Material mat;
        mat.type_ids = { Material_Emissive,0,0,0 };
        mat.color = { _color.x, _color.y, _color.z, 0 };

        return mat;
    }

    Material BVHBuilder::createMirrorMaterial(vec3 _color, float _mirrorness)
    {
        Material mat;
        mat.type_ids = { Material_Mirror,0,0,0 };
        mat.color = { _color.x, _color.y, _color.z, 0 };
        mat.params.x = _mirrorness;

        return mat;
    }

    Material BVHBuilder::createTransparentMaterial(vec3 _color, float _refractionIndice, float _reflectivity)
    {
        Material mat;
        mat.type_ids = { Material_Transparent,0,0,0 };
        mat.color = { _color.x, _color.y, _color.z, 0 };
        mat.params.x = _reflectivity;
        mat.params.y = _refractionIndice;

        return mat;
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

    void BVHBuilder::addTriangleList(u32 _vertexOffset, u32 _numTriangle, const u32 * _indexData, const Material& _mat)
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

    void BVHBuilder::dumpStats() const
    {
        std::cout << "BVHBuilder stats:\n";
        std::cout << " - leaf count: " << m_stats.numLeafs << "\n";
        std::cout << " - mean depth: " << m_stats.meanDepth << "\n";
        std::cout << " - mean triangles: " << m_stats.meanTriangle << "\n";
        std::cout << " - max triangles: " << m_stats.maxTriangle << "\n\n";
    }

    void BVHBuilder::build(u32 _maxDepth, u32 _maxObjPerNode, const Box& _sceneSize)
    {
        m_maxDepth = _maxDepth;
        m_maxObjPerNode = _maxObjPerNode;

        m_stats = {};

        m_nodes.clear();
        m_nodes.reserve(size_t(1) << (m_maxDepth + 1));
        m_nodes.push_back({});
        m_nodes.back().extent = _sceneSize;

        // Compute true AABB of the scene
        Box tightBox = m_objects.empty() ? getAABB(m_triangles[0]) : getAABB(m_objects[0]);
        for (u32 i = 1; i < m_objects.size(); ++i)
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

        // Clamp with user's values
        m_nodes.back().extent.minExtent = linalg::max_(m_nodes.back().extent.minExtent, tightBox.minExtent);
        m_nodes.back().extent.maxExtent = linalg::min_(m_nodes.back().extent.maxExtent, tightBox.maxExtent);

        std::vector<u32> objectsIds(m_objects.size());
        for (u32 i = 0; i < m_objects.size(); ++i)
            objectsIds[i] = i;

        std::vector<u32> triangleIds(m_triangles.size());
        for (u32 i = 0; i < m_triangles.size(); ++i)
            triangleIds[i] = i;

        addObjectsRec(0, objectsIds.begin(), objectsIds.end(), triangleIds.begin(), triangleIds.end(), &m_nodes[0]);

        m_stats.meanDepth /= m_stats.numLeafs;
        m_stats.meanTriangle /= m_stats.numLeafs;

        TIM_ASSERT(m_nodes.size() < g_MaxNodeCount);
    }

    void BVHBuilder::addObjectsRec(u32 _depth, ObjectIt _objectsBegin, ObjectIt _objectsEnd, ObjectIt _trianglesBegin, ObjectIt _trianglesEnd, BVHBuilder::Node* _curNode)
    {
        size_t numObjects = std::distance(_objectsBegin, _objectsEnd) + std::distance(_trianglesBegin, _trianglesEnd);
        if (numObjects <= m_maxObjPerNode || _depth >= m_maxDepth)
        {
            m_stats.numLeafs++;
            m_stats.meanDepth += _depth;
            m_stats.meanTriangle += std::distance(_trianglesBegin, _trianglesEnd);
            m_stats.maxTriangle = std::max<u32>(m_stats.maxTriangle, (u32)std::distance(_trianglesBegin, _trianglesEnd));

            for (auto it = _objectsBegin; it != _objectsEnd && _curNode->primitiveList.size() < (1u << PrimitiveBitCount); ++it)
                _curNode->primitiveList.push_back(*it);

            for (auto it = _trianglesBegin; it != _trianglesEnd && _curNode->triangleList.size() < (1u << TriangleBitCount); ++it)
                _curNode->triangleList.push_back(*it);

            for (size_t i = 0; i < m_lights.size() && _curNode->lightList.size() < (1u << LightBitCount); ++i)
            {
                const Light& light = m_lights[i];
                Sphere sphere = getBoundingSphere(light);

                if(sphereBoxCollision(sphere, _curNode->extent) != CollisionType::Disjoint)
                {
                    _curNode->lightList.push_back(u32(i));
                }
            }
            return;
        }
        else
        {
            _curNode->primitiveList.clear();
            _curNode->triangleList.clear();

            Box leftBox[3], rightBox[3];
            size_t numObjInLeft[3] = { 0,0,0 }, numObjInRight[3] = { 0,0,0 };

            searchBestSplit(_curNode, _objectsBegin, _objectsEnd, _trianglesBegin, _trianglesEnd,
                [](const vec3& step) { return vec3(step.x, 0, 0); }, [](const vec3& _step) { return vec3{ 0, _step.y, _step.z }; },
                leftBox[0], rightBox[0], numObjInLeft[0], numObjInRight[0]);
            searchBestSplit(_curNode, _objectsBegin, _objectsEnd, _trianglesBegin, _trianglesEnd,
                [](const vec3& step) { return vec3(0, step.y, 0); }, [](const vec3& _step) { return vec3{ _step.x, 0, _step.z }; },
                leftBox[1], rightBox[1], numObjInLeft[1], numObjInRight[1]);
            searchBestSplit(_curNode, _objectsBegin, _objectsEnd, _trianglesBegin, _trianglesEnd,
                [](const vec3& step) { return vec3(0, 0, step.z); }, [](const vec3& _step) { return vec3{ _step.x, _step.y, 0 }; },
                leftBox[2], rightBox[2], numObjInLeft[2], numObjInRight[2]);

            // search for best homogeneous split
            auto getBestSplit = [&](u32 s0, u32 s1)
            {
                // best worst case
                return std::max(numObjInLeft[s0], numObjInRight[s0]) < std::max(numObjInLeft[s1], numObjInRight[s1]) ? s0 : s1;
            };

            u32 bestSplit = getBestSplit(getBestSplit(0, 1), 2);
            std::vector<u32> objectLeft; objectLeft.reserve(numObjInLeft[bestSplit]);
            std::vector<u32> objectRight; objectRight.reserve(numObjInRight[bestSplit]);

            std::vector<u32> triangleLeft; triangleLeft.reserve(numObjInLeft[bestSplit]);
            std::vector<u32> triangleRight; triangleRight.reserve(numObjInRight[bestSplit]);

            for (auto it = _objectsBegin; it != _objectsEnd; ++it)
            {
                if (primitiveBoxCollision(m_objects[*it], leftBox[bestSplit]) != CollisionType::Disjoint)
                    objectLeft.push_back(*it);
                if (primitiveBoxCollision(m_objects[*it], rightBox[bestSplit]) != CollisionType::Disjoint)
                    objectRight.push_back(*it);
            }

            for (auto it = _trianglesBegin; it != _trianglesEnd; ++it)
            {
                if (triangleBoxCollision(m_triangles[*it], leftBox[bestSplit]) != CollisionType::Disjoint)
                    triangleLeft.push_back(*it);
                if (triangleBoxCollision(m_triangles[*it], rightBox[bestSplit]) != CollisionType::Disjoint)
                    triangleRight.push_back(*it);
            }

            m_mutex.lock();
            m_nodes.push_back({});
            Node& leftNode = m_nodes.back();

            m_nodes.push_back({});
            Node& rightNode = m_nodes.back();
            m_mutex.unlock();

            leftNode.extent = leftBox[bestSplit];
            leftNode.parent = _curNode;
            _curNode->left = &leftNode;

            rightNode.extent = rightBox[bestSplit];
            rightNode.parent = _curNode;
            _curNode->right = &rightNode;

            leftNode.sibling = &rightNode;
            rightNode.sibling = &leftNode;

            if (_depth < 3)
            {
                std::thread th1([&]()
                {
                    addObjectsRec(_depth + 1, objectLeft.begin(), objectLeft.end(), triangleLeft.begin(), triangleLeft.end(), &leftNode);
                });
                std::thread th2([&]()
                {
                    addObjectsRec(_depth + 1, objectRight.begin(), objectRight.end(), triangleRight.begin(), triangleRight.end(), &rightNode);
                });
                th1.join();
                th2.join();
            }
            else
            {
                addObjectsRec(_depth + 1, objectLeft.begin(), objectLeft.end(), triangleLeft.begin(), triangleLeft.end(), &leftNode);
                addObjectsRec(_depth + 1, objectRight.begin(), objectRight.end(), triangleRight.begin(), triangleRight.end(), &rightNode);
            }
        }
    }

    template<typename Fun1, typename Fun2>
    void BVHBuilder::searchBestSplit(BVHBuilder::Node* _curNode, 
        ObjectIt _objectsBegin, ObjectIt _objectsEnd, ObjectIt _trianglesBegin, ObjectIt _trianglesEnd,
        const Fun1& _movingAxis, const Fun2& _fixedAxis,
        Box& _leftBox, Box& _rightBox, size_t& _numObjInLeft, size_t& _numObjInRight) const
    {
        const u32 g_maxStep = 128;
        size_t numObjects = std::distance(_objectsBegin, _objectsEnd) + std::distance(_trianglesBegin, _trianglesEnd);
        vec3 boxDim = _curNode->extent.maxExtent - _curNode->extent.minExtent;

        size_t curBest_numObjInLeft = std::numeric_limits<size_t>::max();
        size_t curBest_numObjInRight = std::numeric_limits<size_t>::max();

        auto processSplit = [&](const Box& leftBox, const Box& rightBox)
        {
            if (!checkBox(leftBox) || !checkBox(rightBox))
                return;

            size_t numObjInLeft = std::count_if(_objectsBegin, _objectsEnd, [&](u32 _id)
            {
                bool collide = primitiveBoxCollision(m_objects[_id], leftBox) != CollisionType::Disjoint;
                return collide;
            });
            numObjInLeft += std::count_if(_trianglesBegin, _trianglesEnd, [&](u32 _id)
            {
                bool collide = triangleBoxCollision(m_triangles[_id], leftBox) != CollisionType::Disjoint;
                return collide;
            });

            size_t numObjInRight = std::count_if(_objectsBegin, _objectsEnd, [&](u32 _id)
            {
                bool collide = primitiveBoxCollision(m_objects[_id], rightBox) != CollisionType::Disjoint;
                return collide;
            });
            numObjInRight += std::count_if(_trianglesBegin, _trianglesEnd, [&](u32 _id)
            {
                bool collide = triangleBoxCollision(m_triangles[_id], rightBox) != CollisionType::Disjoint;
                return collide;
            });


            if (std::max(numObjInLeft, numObjInRight) < std::max(curBest_numObjInLeft, curBest_numObjInRight))
            {
                _leftBox = leftBox;
                _rightBox = rightBox;
                _numObjInLeft = numObjInLeft;
                _numObjInRight = numObjInRight;
                curBest_numObjInLeft = numObjInLeft;
                curBest_numObjInRight = numObjInRight;
            }
        };

        if (numObjects < g_maxStep)
        {
            vec3 fixedAxis = _fixedAxis(boxDim);
            for (auto it = _trianglesBegin; it < _trianglesEnd; ++it)
            {
                Box aabb = getAABB(m_triangles[*it]);
                aabb.minExtent -= vec3(0.001f, 0.001f, 0.001f);
                aabb.maxExtent += vec3(0.001f, 0.001f, 0.001f);
                {
                    Box leftBox = { _curNode->extent.minExtent, _fixedAxis(_curNode->extent.maxExtent) + _movingAxis(aabb.minExtent) };
                    Box rightBox = { _fixedAxis(_curNode->extent.minExtent) + _movingAxis(aabb.minExtent), _curNode->extent.maxExtent };

                    processSplit(leftBox, rightBox);
                }
                {
                    Box leftBox = { _curNode->extent.minExtent, _fixedAxis(_curNode->extent.maxExtent) + _movingAxis(aabb.maxExtent) };
                    Box rightBox = { _fixedAxis(_curNode->extent.minExtent) + _movingAxis(aabb.maxExtent), _curNode->extent.maxExtent };
                    
                    processSplit(leftBox, rightBox);
                }
            }

            for (auto it = _objectsBegin; it < _objectsEnd; ++it)
            {
                Box aabb = getAABB(m_objects[*it]);
                {
                    Box leftBox = { _curNode->extent.minExtent, _fixedAxis(_curNode->extent.maxExtent) + _movingAxis(aabb.minExtent) };
                    Box rightBox = { _fixedAxis(_curNode->extent.minExtent) + _movingAxis(aabb.minExtent), _curNode->extent.maxExtent };

                    processSplit(leftBox, rightBox);
                }
                {
                    Box leftBox = { _curNode->extent.minExtent, _fixedAxis(_curNode->extent.maxExtent) + _movingAxis(aabb.maxExtent) };
                    Box rightBox = { _fixedAxis(_curNode->extent.minExtent) + _movingAxis(aabb.maxExtent), _curNode->extent.maxExtent };

                    processSplit(leftBox, rightBox);
                }
            }
        }
        else
        {
            vec3 boxStep = boxDim / g_maxStep;
            vec3 step = _movingAxis(boxStep);
            vec3 fixedStep = _fixedAxis(boxDim);

            for (u32 i = 0; i < g_maxStep; ++i)
            {
                Box leftBox = { _curNode->extent.minExtent, _curNode->extent.minExtent + fixedStep + step * i };
                Box rightBox = { _curNode->extent.minExtent + step * i, _curNode->extent.maxExtent };
                processSplit(leftBox, rightBox);
            }
        }
    }

    u32 BVHBuilder::getBvhGpuSize() const
    {
        u32 size = alignUp<u32>((u32)m_nodes.size() * sizeof(PackedBVHNode), m_bufferAlignment);
        size += alignUp<u32>((u32)(m_triangleMaterials.size() + m_objects.size()) * sizeof(Material), m_bufferAlignment);
        size += alignUp<u32>((u32)m_triangles.size() * sizeof(Triangle), m_bufferAlignment);
        size += alignUp<u32>((u32)m_objects.size() * sizeof(PackedPrimitive), m_bufferAlignment);
        size += alignUp<u32>((u32)m_lights.size() * sizeof(PackedLight), m_bufferAlignment);
        for (const BVHBuilder::Node& n : m_nodes)
        {
            size += (u32)(1 + n.triangleList.size() + n.primitiveList.size() + n.lightList.size()) * sizeof(u32);
        }
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
        u32 leftIndex = _node.left ? u32(_node.left - &m_nodes[0]) : InvalidNodeId;
        u32 rightIndex = _node.right ? u32(_node.right - &m_nodes[0]) : InvalidNodeId;
        u32 parentIndex = _node.parent ? u32(_node.parent - &m_nodes[0]) : InvalidNodeId;
        u32 siblingIndex = _node.sibling ? u32(_node.sibling - &m_nodes[0]) : InvalidNodeId;
        
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

    void BVHBuilder::fillGpuBuffer(void* _data, uvec2& _triangleOffsetRange, uvec2& _primitiveOffsetRange, uvec2& _materialOffsetRange, uvec2& _lightOffsetRange, uvec2& _nodeOffsetRange, uvec2& _leafDataOffsetRange)
    {
        ubyte* out = (ubyte*)_data;
        ubyte* prevout = out;

        // Store triangle data
        u32 triangleBufferSize = u32(m_triangles.size() * sizeof(Triangle));
        _triangleOffsetRange = { (u32)std::distance((ubyte*)_data, out), triangleBufferSize };
        _triangleOffsetRange.y = std::max(m_bufferAlignment, _triangleOffsetRange.y);
        
        for (u32 i = 0; i < m_triangles.size(); ++i)
        {
            Triangle * tri = reinterpret_cast<Triangle*>(out);
            *tri = m_triangles[i];
        
            out += sizeof(Triangle);
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
        u32 nodeBufferSize = u32(m_nodes.size() * sizeof(PackedBVHNode));
        _nodeOffsetRange = { (u32)std::distance((ubyte*)_data, out), nodeBufferSize };
        _nodeOffsetRange.y = std::max(m_bufferAlignment, _nodeOffsetRange.y);

        u32* objectListBegin = reinterpret_cast<u32*>(out + alignUp(nodeBufferSize, m_bufferAlignment));
        u32* objectListCurPtr = objectListBegin;

        for (const BVHBuilder::Node& n : m_nodes)
        {
            PackedBVHNode* node = reinterpret_cast<PackedBVHNode*>(out);
            u32 leafDataIndex = (n.triangleList.size() + n.primitiveList.size()) == 0 ? u32(-1) : u32(objectListCurPtr - objectListBegin);
            packNodeData(node, n, leafDataIndex);

            static_assert(TriangleBitCount + PrimitiveBitCount + LightBitCount == 32);
            TIM_ASSERT(u32(n.triangleList.size()) < (1 << TriangleBitCount));
            TIM_ASSERT(u32(n.primitiveList.size()) < (1 << PrimitiveBitCount));
            TIM_ASSERT(u32(n.lightList.size()) < (1 << LightBitCount));

            u32 packedTriObjLightCount = u32(n.triangleList.size()) + (u32(n.primitiveList.size()) << TriangleBitCount) + (u32(n.lightList.size()) << (TriangleBitCount + PrimitiveBitCount));

            *objectListCurPtr = packedTriObjLightCount;
            ++objectListCurPtr;
            memcpy(objectListCurPtr, n.triangleList.data(), sizeof(u32) * n.triangleList.size());
            objectListCurPtr += n.triangleList.size();
            memcpy(objectListCurPtr, n.primitiveList.data(), sizeof(u32) * n.primitiveList.size());
            objectListCurPtr += n.primitiveList.size();
            memcpy(objectListCurPtr, n.lightList.data(), sizeof(u32) * n.lightList.size());
            objectListCurPtr += n.lightList.size();
            out += sizeof(PackedBVHNode);
        }

        _leafDataOffsetRange = { (u32)std::distance((ubyte*)_data, (ubyte*)objectListBegin), (u32)std::distance((ubyte*)objectListBegin, (ubyte*)objectListCurPtr) };
    }
}

