#pragma once
#include "rtDevice/public/IRenderer.h"

namespace tim
{
    class BVHBuilder;
    class BVHGeometry;

    class Scene
    {
    public:
        Scene(BVHGeometry& _geometryBuffer);
        ~Scene() = default;

        void build(BVHBuilder* _bvh);

    private:
        BVHGeometry& m_geometryBuffer;

        void addOBJ(const char* _path, vec3 _pos, BVHBuilder* _builder);

    };
}