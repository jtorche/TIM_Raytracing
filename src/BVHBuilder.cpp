#include "BVHBuilder.h"
#include "timCore/Common.h"

using namespace tim;

namespace
{
    enum class CollisionType { Disjoint, Intersect, Contained };
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

    CollisionType primitiveBoxCollision(const Primitive& _prim, const Box& _box)
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

    CollisionType primitiveSphereCollision(const Primitive& _prim, const Sphere& _sphere)
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

    Box getAABB(const Primitive& _prim)
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

    float getBoxVolume(const Box& _box)
    {
        vec3 dim = _box.maxExtent - _box.minExtent;
        return dim.x * dim.y * dim.z;
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

Material BVHBuilder::createTransparentMaterial(vec3 _color, float _refractionIndice, float _opacity)
{
    Material mat;
    mat.type_ids = { Material_Transparent,0,0,0 };
    mat.color = { _color.x, _color.y, _color.z, 0 };
    mat.params.x = _opacity;
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

void BVHBuilder::addSphereLight(const SphereLight& _light)
{
    m_lights.push_back({ _light });
}

void BVHBuilder::addAreaLight(const AreaLight& _light)
{
    m_lights.push_back({ _light });
}

void BVHBuilder::build(u32 _maxDepth, u32 _maxObjPerNode, const Box& _sceneSize)
{
    m_maxDepth = _maxDepth;
    m_maxObjPerNode = _maxObjPerNode;

    TIM_ASSERT(m_objects.size() < 0x7FFF);

    m_nodes.clear();
    m_nodes.reserve(size_t(1) << (m_maxDepth + 1));
    m_nodes.push_back({});
    m_nodes.back().extent = _sceneSize;

    // Compute true AABB of the scene
    Box tightBox = getAABB(m_objects[0]);
    for (u32 i = 1; i < m_objects.size(); ++i)
    {
        Box box = getAABB(m_objects[i]);
        tightBox.minExtent = linalg::min_(tightBox.minExtent, box.minExtent);
        tightBox.maxExtent = linalg::max_(tightBox.maxExtent, box.maxExtent);
    }

    // Clamp with user's values
    m_nodes.back().extent.minExtent = linalg::max_(m_nodes.back().extent.minExtent, tightBox.minExtent);
    m_nodes.back().extent.maxExtent = linalg::min_(m_nodes.back().extent.maxExtent, tightBox.maxExtent);

    std::vector<u32> objectsIds(m_objects.size());
    for (u32 i = 0; i < m_objects.size(); ++i)
        objectsIds[i] = i;

    addObjectsRec(0, objectsIds.begin(), objectsIds.end(), &m_nodes[0]);
}

void BVHBuilder::addObjectsRec(u32 _depth, ObjectIt _objectsBegin, ObjectIt _objectsEnd, BVHBuilder::Node* _curNode)
{
    constexpr u32 g_maxObject = 0x7FFF;

    size_t numObjects = std::distance(_objectsBegin, _objectsEnd);
    if (numObjects <= m_maxObjPerNode || _depth >= m_maxDepth)
    {
        for (auto it = _objectsBegin; it != _objectsEnd; ++it)
            _curNode->primitiveList.push_back(*it);

        for (size_t i = 0; i < m_lights.size(); ++i)
        {
            const Light& light = m_lights[i];
            Sphere sphere = getBoundingSphere(light);

            if (std::any_of(_objectsBegin, _objectsEnd, [&](u32 _objIndex)
                {
                    return primitiveSphereCollision(m_objects[_objIndex], sphere) != CollisionType::Disjoint;
                }))
            {
                _curNode->lightList.push_back(u32(i));
            }
        }
        return;
    }
    else
    {
        _curNode->primitiveList.clear();

        Box leftBox[3], rightBox[3];
        size_t numObjInLeft[3] = { 0,0,0 }, numObjInRight[3] = { 0,0,0 };

        searchBestSplit(_curNode, _objectsBegin, _objectsEnd,
            [](const vec3& step) { return vec3(step.x, 0, 0); }, [](const vec3& _step) { return vec3{ 0, _step.y, _step.z }; },
            leftBox[0], rightBox[0], numObjInLeft[0], numObjInRight[0]);
        searchBestSplit(_curNode, _objectsBegin, _objectsEnd,
            [](const vec3& step) { return vec3(0, step.y, 0); }, [](const vec3& _step) { return vec3{ _step.x, 0, _step.z }; },
            leftBox[1], rightBox[1], numObjInLeft[1], numObjInRight[1]);
        searchBestSplit(_curNode, _objectsBegin, _objectsEnd,
            [](const vec3& step) { return vec3(0, 0, step.z); }, [](const vec3& _step) { return vec3{ _step.x, _step.y, 0 }; },
            leftBox[2], rightBox[2], numObjInLeft[2], numObjInRight[2]);

        // search for best homogeneous split
        auto getBestSplit = [&](u32 s0, u32 s1)
        {
            if (numObjInLeft[s0] + numObjInRight[s0] < numObjInLeft[s1] + numObjInRight[s1])
                return s0;

            if (absolute_difference(numObjInLeft[s0], numObjInRight[s0]) < absolute_difference(numObjInLeft[s1], numObjInRight[s1]))
                return s0;

            if (fabs(getBoxVolume(leftBox[s0]) - getBoxVolume(rightBox[s0])) < fabs(getBoxVolume(leftBox[s1]) - getBoxVolume(rightBox[s1])))
                return s0;

            return s1;
        };

        u32 bestSplit = getBestSplit(getBestSplit(0, 1), 2);
        std::vector<u32> objectLeft; objectLeft.reserve(numObjInLeft[bestSplit]);
        std::vector<u32> objectRight; objectRight.reserve(numObjInRight[bestSplit]);

        for (auto it = _objectsBegin; it != _objectsEnd; ++it)
        {
            if (primitiveBoxCollision(m_objects[*it], leftBox[bestSplit]) != CollisionType::Disjoint)
                objectLeft.push_back(*it);
            if (primitiveBoxCollision(m_objects[*it], rightBox[bestSplit]) != CollisionType::Disjoint)
                objectRight.push_back(*it);
        }

        TIM_ASSERT(m_nodes.size() < g_maxObject);
        m_nodes.push_back({});
        Node& leftNode = m_nodes.back();
        leftNode.extent = leftBox[bestSplit];
        leftNode.parent = _curNode;
        _curNode->left = &leftNode;

        TIM_ASSERT(m_nodes.size() < g_maxObject);
        m_nodes.push_back({});
        Node& rightNode = m_nodes.back();
        rightNode.extent = rightBox[bestSplit];
        rightNode.parent = _curNode;
        _curNode->right = &rightNode;

        leftNode.sibling = &rightNode;
        rightNode.sibling = &leftNode;
        addObjectsRec(_depth + 1, objectLeft.begin(), objectLeft.end(), &leftNode);
        addObjectsRec(_depth + 1, objectRight.begin(), objectRight.end(), &rightNode);
    }
}

template<typename Fun1, typename Fun2>
void BVHBuilder::searchBestSplit(BVHBuilder::Node* _curNode, ObjectIt _objectsBegin, ObjectIt _objectsEnd,
    const Fun1& _computeStep, const Fun2& _computeFixedStep,
    Box& _leftBox, Box& _rightBox, size_t& _numObjInLeft, size_t& _numObjInRight) const
{
    const u32 NumSplit = 128;
    size_t numObjects = std::distance(_objectsBegin, _objectsEnd);
    const bool isEven = numObjects % 2 == 0;

    vec3 boxDim = _curNode->extent.maxExtent - _curNode->extent.minExtent;
    vec3 boxStep = boxDim / NumSplit;

    bool best_objectRepartitioHeuristic = false;
    bool best_noEmptyHeuristic = false;
    size_t best_objectSum = 0xFFFFffff;
    float best_volumeDiff = std::numeric_limits<float>::max();

    auto isBetter = [&](bool _objectRepartitioHeuristic, bool _nonEmptyHeuristic, size_t _objectSum, float _volumeDiff)
    {
        if (_objectRepartitioHeuristic != best_objectRepartitioHeuristic)
            return _objectRepartitioHeuristic;
        if (_nonEmptyHeuristic != best_noEmptyHeuristic)
            return _nonEmptyHeuristic;
        if (_objectSum != best_objectSum)
            return _objectSum < best_objectSum;
        
        return _volumeDiff < best_volumeDiff;
    };

    vec3 step = _computeStep(boxStep);
    vec3 fixedStep = _computeFixedStep(boxDim);

    for (u32 i = 0; i < NumSplit; ++i)
    {
        Box leftBox = { _curNode->extent.minExtent, _curNode->extent.minExtent + fixedStep + step * i };
        Box rightBox = { _curNode->extent.minExtent + step * i, _curNode->extent.maxExtent };

        size_t numObjInLeft = std::count_if(_objectsBegin, _objectsEnd, [&](u32 _id)
            {
                bool collide = boxBoxCollision(leftBox, getAABB(m_objects[_id])) != CollisionType::Disjoint;
                return collide;
            });

        size_t numObjInRight = std::count_if(_objectsBegin, _objectsEnd, [&](u32 _id)
            {
                bool collide = boxBoxCollision(rightBox, getAABB(m_objects[_id])) != CollisionType::Disjoint;
                return collide;
            });
        
        size_t objectSum = numObjInLeft + numObjInRight;
        bool noEmptyHeuristic = numObjInLeft != 0 && numObjInRight != 0;
        bool repartitionHeuristic = objectSum == numObjects && (isEven ? numObjInLeft == numObjInRight : numObjInRight == numObjInLeft + 1);
        float volumeDiff = fabsf(getBoxVolume(leftBox) - getBoxVolume(rightBox));

        if (isBetter(repartitionHeuristic, noEmptyHeuristic, objectSum, volumeDiff))
        {
            _leftBox = leftBox;
            _rightBox = rightBox;
            _numObjInLeft = numObjInLeft;
            _numObjInRight = numObjInRight;

            best_objectRepartitioHeuristic = repartitionHeuristic;
            best_noEmptyHeuristic = noEmptyHeuristic;
            best_objectSum = objectSum;
            best_volumeDiff = volumeDiff;
        }
    }
}

u32 BVHBuilder::getBvhGpuSize() const
{
    u32 size = tim::alignUp<u32>((u32)m_nodes.size() * sizeof(PackedBVHNode), m_bufferAlignment);
    size += tim::alignUp<u32>((u32)m_objects.size() * sizeof(Material), m_bufferAlignment);
    size += tim::alignUp<u32>((u32)m_objects.size() * sizeof(PackedPrimitive), m_bufferAlignment);
    size += tim::alignUp<u32>((u32)m_lights.size() * sizeof(PackedLight), m_bufferAlignment);
    for (const BVHBuilder::Node& n : m_nodes)
    {
        size += (u32)(n.primitiveList.size() + n.lightList.size()) * sizeof(u32);
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
    constexpr u16 InvalidNodeId = 0x7FFF;
    u16 leftIndex = _node.left ? u16(_node.left - &m_nodes[0]) : InvalidNodeId;
    u16 rightIndex = _node.right ? u16(_node.right - &m_nodes[0]) : InvalidNodeId;
    u16 parentIndex = _node.parent ? u16(_node.parent - &m_nodes[0]) : InvalidNodeId;
    u16 siblingIndex = _node.sibling ? u16(_node.sibling - &m_nodes[0]) : InvalidNodeId;
    TIM_ASSERT((leftIndex == InvalidNodeId && rightIndex == InvalidNodeId) || (leftIndex != InvalidNodeId && rightIndex != InvalidNodeId));

    auto isLeaf = [](const BVHBuilder::Node* node) { return node ? node->left == nullptr : false; };
    auto hasParent = [](const BVHBuilder::Node* node) { return node ? node->parent != nullptr : false; };

    // add bit to fast check if leaf
    siblingIndex = isLeaf(_node.sibling) ? 0x8000 | siblingIndex : siblingIndex;
    leftIndex = isLeaf(_node.left) ? 0x8000 | leftIndex : leftIndex;
    rightIndex = isLeaf(_node.right) ? 0x8000 | rightIndex : rightIndex;

    u32 packedObjLightCount = u32(_node.primitiveList.size()) + (u32(_node.lightList.size()) << 16);
    _outNode->nid = uvec4(parentIndex + (siblingIndex << 16), leftIndex + (rightIndex << 16), _leafDataOffset, packedObjLightCount);

    TIM_ASSERT((leftIndex == InvalidNodeId && rightIndex == InvalidNodeId) || (leftIndex != InvalidNodeId && rightIndex != InvalidNodeId));
    if (leftIndex != InvalidNodeId)
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

void BVHBuilder::fillGpuBuffer(void* _data, uvec2& _primitiveOffsetRange, uvec2& _materialOffsetRange, uvec2& _lightOffsetRange, uvec2& _nodeOffsetRange, uvec2& _leafDataOffsetRange)
{
    ubyte* out = (ubyte*)_data;
    ubyte* prevout = out;

    // Store primitive data
    u32 objectBufferSize = u32(m_objects.size() * sizeof(PackedPrimitive));
    _primitiveOffsetRange = { (u32)std::distance((ubyte*)_data, out), objectBufferSize };

    for (u32 i = 0; i < m_objects.size(); ++i)
    {
        PackedPrimitive* prim = reinterpret_cast<PackedPrimitive*>(out);
        const u32 materialId = i;
        prim->iparam = m_objects[i].type + (materialId << 16);
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
    }
    out = prevout + alignUp(objectBufferSize, m_bufferAlignment);

    // Store material data
    u32 materialBufferSize = u32(m_objects.size() * sizeof(Material));
    _materialOffsetRange = { (u32)std::distance((ubyte*)_data, out), materialBufferSize };
    prevout = out;
    for (u32 i = 0; i < m_objects.size(); ++i)
    {
        *reinterpret_cast<Material*>(out) = m_objects[i].material;
        out += sizeof(Material);
    }
    out = prevout + alignUp(materialBufferSize, m_bufferAlignment);

    // Store light data
    u32 lightBufferSize = u32(m_lights.size() * sizeof(PackedLight));
    _lightOffsetRange = { (u32)std::distance((ubyte*)_data, out), lightBufferSize };
    prevout = out;
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

    // Store node data
    u32 nodeBufferSize = u32(m_nodes.size() * sizeof(PackedBVHNode));
    _nodeOffsetRange = { (u32)std::distance((ubyte*)_data, out), nodeBufferSize };

    u32* primitiveListBegin = reinterpret_cast<u32*>(out + alignUp(nodeBufferSize, m_bufferAlignment));
    u32* primitiveListCurPtr = primitiveListBegin;

    TIM_ASSERT(m_nodes.size() < 0x7FFF);
    for (const BVHBuilder::Node& n : m_nodes)
    {
        PackedBVHNode* node = reinterpret_cast<PackedBVHNode*>(out);
        u32 leafDataIndex = n.primitiveList.empty() ? u32(-1) : u32(primitiveListCurPtr - primitiveListBegin);
        packNodeData(node, n, leafDataIndex);

        memcpy(primitiveListCurPtr, n.primitiveList.data(), sizeof(u32) * n.primitiveList.size());
        primitiveListCurPtr += n.primitiveList.size();
        memcpy(primitiveListCurPtr, n.lightList.data(), sizeof(u32) * n.lightList.size());
        primitiveListCurPtr += n.lightList.size();
        out += sizeof(PackedBVHNode);
    }

    _leafDataOffsetRange = { (u32)std::distance((ubyte*)_data, (ubyte*)primitiveListBegin), (u32)std::distance((ubyte*)primitiveListBegin, (ubyte*)primitiveListCurPtr) };
}

