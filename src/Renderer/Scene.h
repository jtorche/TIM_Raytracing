#pragma once
#include "rtDevice/public/IRenderer.h"
#include <filesystem>

struct Material;

namespace tim
{
    namespace fs = std::filesystem;
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

        void addOBJ(const fs::path& _path, vec3 _pos, vec3 _scale, BVHBuilder* _builder, const Material& _mat, bool _swapYZ = false);
        void addOBJWithMtl(const fs::path& _path, vec3 _pos, vec3 _scale, BVHBuilder* _builder, bool _swapYZ = false);

        void addOBJInner(const fs::path& _path, vec3 _pos, vec3 _scale, BVHBuilder* _builder, const Material* _mat, bool _swapYZ = false);

    };
}