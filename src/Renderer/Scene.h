#pragma once
#include "rtDevice/public/IRenderer.h"

struct Material;

namespace tim
{
    class BVHBuilder;
    class BVHGeometry;
    class TextureManager;

    class Scene
    {
    public:
        Scene(BVHGeometry& _geometryBuffer, TextureManager& _texManager);
        ~Scene() = default;

        void build(BVHBuilder* _bvh);

    private:
        BVHGeometry& m_geometryBuffer;
        TextureManager& m_texManager;

        void addOBJ(const char* _path, vec3 _pos, vec3 _scale, BVHBuilder* _builder, const Material& _mat);

    };
}