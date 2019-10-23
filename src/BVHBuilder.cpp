#include "BVHBuilder.h"
#include "timCore/Common.h"

using namespace tim;

namespace
{
    enum class CollisionType { Disjoint, Intersect, Contained };
    CollisionType sphereBoxCollision(const Box& _box, const Sphere& _sphere)
    {
        vec3 closestPointInAabb = linalg::min_(linalg::max_(_sphere.center, _box.minExtent), _box.maxExtent);
        float distanceSquared = linalg::length2(closestPointInAabb - _sphere.center);

        return distanceSquared < (_sphere.radius * _sphere.radius) ? CollisionType::Intersect : CollisionType::Disjoint;
    }

    CollisionType boxBoxCollision(const Box& _box1, const Box& _box2)
    {
        if (_box1.minExtent.x > _box2.maxExtent.x || _box1.minExtent.y > _box2.maxExtent.y || _box1.minExtent.z > _box2.maxExtent.z ||
            _box2.minExtent.x > _box1.maxExtent.x || _box2.minExtent.y > _box1.maxExtent.y || _box2.minExtent.z > _box1.maxExtent.z)
        {
            CollisionType::Disjoint;
        }

        if (_box1.minExtent.x > _box2.minExtent.x&& _box1.minExtent.y > _box2.minExtent.y&& _box1.minExtent.z > _box2.minExtent.z&&
            _box1.maxExtent.x < _box2.maxExtent.x && _box1.maxExtent.y < _box2.maxExtent.y && _box1.maxExtent.z < _box2.maxExtent.z)
        {
            return CollisionType::Contained;
        }

        return CollisionType::Intersect;
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
}

void BVHBuilder::addSphere(const Sphere& _sphere)
{
    m_objects.push_back({ _sphere });
    m_objects.back().m_sphere.invRadius = 1.f / _sphere.radius;
}

void BVHBuilder::build(const Box& _sceneSize)
{
    TIM_ASSERT(m_objects.size() < u16(-1));

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
    size_t numObjects = std::distance(_objectsBegin, _objectsEnd);
    if (numObjects <= m_maxObjPerNode || _depth >= m_maxDepth)
    {
        for (auto it = _objectsBegin; it != _objectsEnd; ++it)
        {
            _curNode->primitiveList.push_back(*it);
            return;
        }
    }
    else
    {
        _curNode->primitiveList.clear();

        Box leftBox[3], rightBox[3];
        size_t numObjInLeft[3] = { 0,0,0 }, numObjInRight[3] = { 0,0,0 };

        searchBestSplit(_curNode, _objectsBegin, _objectsEnd, [](vec3 _step) { return _step.x; },
                        leftBox[0], rightBox[0], numObjInLeft[0], numObjInRight[0]);
        searchBestSplit(_curNode, _objectsBegin, _objectsEnd, [](vec3 _step) { return _step.y; },
                        leftBox[1], rightBox[1], numObjInLeft[1], numObjInRight[1]);
        searchBestSplit(_curNode, _objectsBegin, _objectsEnd, [](vec3 _step) { return _step.z; },
                        leftBox[2], rightBox[2], numObjInLeft[2], numObjInRight[2]);

        // search for best homogeneous split
        u32 bestSplit = 0;
        bestSplit = numObjInLeft[bestSplit] * numObjInRight[bestSplit] > numObjInLeft[1] * numObjInRight[1] ? bestSplit : 1;
        bestSplit = numObjInLeft[bestSplit] * numObjInRight[bestSplit] > numObjInLeft[2] * numObjInRight[2] ? bestSplit : 2;

        std::vector<u32> objectLeft; objectLeft.reserve(numObjInLeft[bestSplit]);
        std::vector<u32> objectRight; objectRight.reserve(numObjInRight[bestSplit]);

        for (auto it = _objectsBegin; it != _objectsEnd; ++it)
        {
            if (boxBoxCollision(leftBox[bestSplit], getAABB(m_objects[*it])) != CollisionType::Disjoint)
                objectLeft.push_back(*it);
            if (boxBoxCollision(rightBox[bestSplit], getAABB(m_objects[*it])) != CollisionType::Disjoint)
                objectRight.push_back(*it);
        }

        TIM_ASSERT(m_nodes.size() < m_nodes.capacity());
        m_nodes.push_back({});
        m_nodes.back().extent = leftBox[bestSplit];
        m_nodes.back().parent = _curNode;
        _curNode->left = &m_nodes.back();
        addObjectsRec(_depth + 1, objectLeft.begin(), objectLeft.end(), &m_nodes.back());

        m_nodes.push_back({});
        m_nodes.back().extent = rightBox[bestSplit];
        m_nodes.back().parent = _curNode;
        _curNode->right = &m_nodes.back();
        addObjectsRec(_depth + 1, objectRight.begin(), objectRight.end(), &m_nodes.back());
    }
}

template<typename Fun>
void BVHBuilder::searchBestSplit(BVHBuilder::Node* _curNode, ObjectIt _objectsBegin, ObjectIt _objectsEnd, const Fun& _extractStep,
                                 Box& _leftBox, Box& _rightBox, size_t& _numObjInLeft, size_t& _numObjInRight) const
{
    const u32 NumSplit = 32;
    size_t numObjects = std::distance(_objectsBegin, _objectsEnd);

    vec3 boxDim = _curNode->extent.maxExtent - _curNode->extent.minExtent;
    vec3 boxStep = boxDim / NumSplit;

    for (u32 i = 0; i < NumSplit; ++i)
    {
        vec3 newMaxExtent = _curNode->extent.minExtent + _extractStep(boxStep) * i;
        _leftBox = { _curNode->extent.minExtent, newMaxExtent };
        _rightBox = { newMaxExtent, _curNode->extent.maxExtent };

        _numObjInLeft = std::count_if(_objectsBegin, _objectsEnd, [&](u32 _id)
            {
                bool collide = boxBoxCollision(_leftBox, getAABB(m_objects[_id])) != CollisionType::Disjoint;
                return collide;
            });

        _numObjInRight = std::count_if(_objectsBegin, _objectsEnd, [&](u32 _id)
            {
                bool collide = boxBoxCollision(_rightBox, getAABB(m_objects[_id])) != CollisionType::Disjoint;
                return collide;
            });

        if (_numObjInLeft + _numObjInRight == numObjects || _numObjInLeft > _numObjInRight)
            break;
    }
}

u32 BVHBuilder::getBvhGpuSize() const
{
    u32 size = (u32)m_nodes.size() * sizeof(PackedBVHNode);
    size += (u32)m_objects.size() * sizeof(PackedPrimitive);
    for (const BVHBuilder::Node& n : m_nodes)
    {
        size += (u32)n.primitiveList.size() * sizeof(u32);
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
}

void BVHBuilder::fillGpuBuffer(void* _data, uvec2& _primitiveOffsetRange, uvec2 _nodeOffsetRange, uvec2& _leafDataOffsetRange)
{
    ubyte* out = (ubyte*)_data;
    for (u32 i = 0; i < m_objects.size(); ++i)
    {
        PackedPrimitive* prim = reinterpret_cast<PackedPrimitive*>(out);
        prim->iparam = m_objects[i].type;
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

    u32* primitiveListBegin = reinterpret_cast<u32*>(out + ((u32)m_nodes.size() * sizeof(PackedBVHNode)));
    u32* primitiveListCurPtr = primitiveListBegin;

    for (const BVHBuilder::Node& n : m_nodes)
    {
        PackedBVHNode * node = reinterpret_cast<PackedBVHNode*>(out);
        u16 leftIndex = n.left ? u16(n.left - &m_nodes[0]) : 0xFFFF;
        u16 rightIndex = n.right ? u16(n.right - &m_nodes[0]) : 0xFFFF;
        node->child_lr = leftIndex + (rightIndex << 16);
        n.extent.minExtent.assign(node->minExtent);
        n.extent.maxExtent.assign(node->maxExtent);

        u16 parentIndex = n.parent ? u16(n.parent - &m_nodes[0]) : 0xFFFF;
        node->numObjects_parent = u32(n.primitiveList.size()) + (parentIndex << 16);
        node->objectOffset = n.primitiveList.empty() ? u32(-1) : u32(primitiveListCurPtr - primitiveListBegin);
        memcpy(primitiveListCurPtr, n.primitiveList.data(), n.primitiveList.size());
        primitiveListCurPtr += n.primitiveList.size();
        out += sizeof(PackedBVHNode);
    }

    _primitiveOffsetRange = { 0, (u32)m_objects.size() * sizeof(PackedPrimitive) };
    _nodeOffsetRange =      { _primitiveOffsetRange.y, (u32)m_nodes.size() * sizeof(PackedBVHNode) };
    _leafDataOffsetRange =  { _primitiveOffsetRange.y + _nodeOffsetRange.y, sizeof(u32) * u32(primitiveListCurPtr - primitiveListBegin) };
}

