#pragma once
#include "rtDevice/public/IRenderer.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "resourceAllocator.h"

namespace tim
{
    class SimpleCamera;
    class BVHBuilder;
    class BVHGeometry;
    class TextureManager;

    struct SunData
    {
        vec3 sunDir = vec3(0.3f, 0.3f, -1);
        vec3 sunColor = vec3(1,1,1);
    };

    class RayTracingPass
    {
    public:
        RayTracingPass(IRenderer* _renderer, IRenderContext* _context, ResourceAllocator& _allocator, TextureManager& _texManager);
        ~RayTracingPass();

        void rebuildBvh(u32 _maxBlasPerNode, u32 _maxTriPerNode, bool _useBlas);
        void setBounceRecursionDepth(u32 _depth);
        void setFrameBufferSize(uvec2 _res);
        void draw(ImageHandle _outputBuffer, const SimpleCamera& _camera);

    private:
        void drawBounce(u32 _depth, BufferView _passData, BufferHandle _inputRayBuffer, ImageHandle _outputImage);
        u32 getRayStorageBufferSize() const;

    private:
        u32 m_rayBounceRecursionDepth;
        uvec2 m_frameSize;
        IRenderer* m_renderer = nullptr;
        IRenderContext* m_context = nullptr;
        ResourceAllocator& m_resourceAllocator;
        TextureManager& m_textureManager;

        SunData m_sunData;
        std::unique_ptr<BVHBuilder> m_bvh;
        std::unique_ptr<BVHGeometry> m_geometryBuffer;
        BufferHandle m_bvhBuffer;
        uvec2 m_bvhTriangleOffsetRange;
        uvec2 m_bvhPrimitiveOffsetRange;
        uvec2 m_bvhMaterialOffsetRange;
        uvec2 m_bvhLightOffsetRange;
        uvec2 m_bvhNodeOffsetRange;
        uvec2 m_bvhLeafDataOffsetRange;
        uvec2 m_bvhBlasHeaderDataOffsetRange;
    };
}