#pragma once
#include "rtDevice/public/IRenderer.h"
#include "LightProbField.h"
#include "Shaders/core/primitive_cpp.glsl"
#include <filesystem>

struct Material;

namespace tim
{
    namespace fs = std::filesystem;
    class IRenderer;
    class BVHBuilder;
    class BVHGeometry;
    class BVHData;
    class TextureManager;
    struct BufferBinding;
    struct BVHBuildParameters;

    struct SunData
    {
        vec3 sunDir = vec3(0.3f, 0.3f, -1);
        vec3 sunColor = vec3(3, 3, 3);
    };

    class Scene
    {
    public:
        Scene(IRenderer* _renderer, TextureManager& _texManager);
        ~Scene();

        void build(const BVHBuildParameters& _bvhParams, const BVHBuildParameters& _tlasParams, bool _useTlasBlas);
        void fillGeometryBufferBindings(std::vector<BufferBinding>& _bindings) const;

        void setSunData(const SunData& _data) { m_sunData = _data; }
        const SunData getSunData() const { return m_sunData; }
        const BVHData& getBVH() const { return *m_bvhData; }
        const LightProbField& getLPF() const { return m_lightProbField; }

        u32 getPrimitivesCount() const;
        u32 getTrianglesCount() const;
        u32 getBlasInstancesCount() const;
        u32 getLightsCount() const;
        u32 getNodesCount() const;
        Box getAABB() const;
        bool useTlas() const;

        void setLightProbFieldResolution(uvec3 _res);

    private:
        IRenderer* m_renderer;
        TextureManager& m_texManager;
        std::unique_ptr<BVHGeometry> m_geometryBuffer;
        std::unique_ptr<BVHBuilder> m_bvh;
        std::unique_ptr<BVHData> m_bvhData;
        bool m_useTlas = false;

        SunData m_sunData;
        LightProbField m_lightProbField;

    private:
        void addOBJ(const fs::path& _path, vec3 _pos, vec3 _scale, BVHBuilder* _builder, const Material& _mat, bool _swapYZ = false);
        void addOBJWithMtl(const fs::path& _path, vec3 _pos, vec3 _scale, BVHBuilder* _builder, bool _swapYZ = false);
        void addOBJInner(const fs::path& _path, vec3 _pos, vec3 _scale, BVHBuilder* _builder, const Material* _mat, bool _swapYZ = false);

        void loadBlas(const fs::path& _path, vec3 _pos, vec3 _scale, std::vector<std::unique_ptr<BVHBuilder>>& _blas, const Material* _mat = nullptr, bool _swapYZ = false);
    };
}