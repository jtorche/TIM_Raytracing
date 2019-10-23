#include "src/timCore/type.h"
#include <vector>

using namespace tim;
#include "Shaders/primitive_cpp.fxh"

struct Primitive
{
    Primitive(Sphere _sphere) : type{ Primitive_Sphere }, m_sphere{ _sphere } {}
    Primitive(Box _box) : type{ Primitive_AABB }, m_aabb{ _box } {}

    u32 type = 0;
    union
    {
        Sphere  m_sphere = { {0,0,0}, 1, 1 };
        Box     m_aabb;
    };
};

class BVHBuilder
{
public:
    BVHBuilder(u32 _maxDepth = 5, u32 _maxObjPerNode = 8) : m_maxDepth{ _maxDepth }, m_maxObjPerNode{ _maxObjPerNode } {}

    void addSphere(const Sphere&);
    void build(const Box& _sceneSize);

    u32 getBvhGpuSize() const;

    // return offset to root node + offset to first primitive list of leafs, offset 0 is for primitive data
    void fillGpuBuffer(void* _data, uvec2& _primitiveOffsetRange, uvec2 _nodeOffsetRange, uvec2& m_leafDataOffsetRange);

private:
    struct Node;
    using ObjectIt = std::vector<u32>::iterator;
    void addObjectsRec(u32 _depth, ObjectIt _objectsBegin, ObjectIt _objectsEnd, Node * _curNode);

    template<typename Fun>
    void searchBestSplit(Node * _curNode, ObjectIt _objectsBegin, ObjectIt _objectsEnd, const Fun& _extractStep,
                         Box& _leftBox, Box& _rightBox, size_t& _numObjInLeft, size_t& _numObjInRight) const;
private:
    u32 m_maxDepth, m_maxObjPerNode;
    std::vector<Sphere> m_spheres;

    struct Node
    {
        Box extent;
        Node* parent = nullptr;
        Node* left = nullptr;
        Node* right = nullptr;
        std::vector<u32> primitiveList; // only if leaf
    };

    std::vector<Node> m_nodes;
    std::vector<Primitive> m_objects;
};