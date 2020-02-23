#pragma once
#include "rtDevice/public/IRenderer.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "resourceAllocator.h"

namespace tim
{
    class SimpleCamera;
    class BVHBuilder;
    class BVHGeometry;

    class RayTracingPass
    {
    public:
        RayTracingPass(IRenderer* _renderer, IRenderContext* _context, ResourceAllocator& _allocator);
        ~RayTracingPass();

        void rebuildBvh(u32 _maxDepth, u32 _maxObjPerNode);
        void setFrameBufferSize(uvec2 _res);
        void draw(ImageHandle _outputBuffer, const SimpleCamera& _camera);

    private:
        void drawBounce(u32 _depth, BufferView _passData, BufferHandle _inputRayBuffer, ImageHandle _outputImage);
        u32 RayTracingPass::getRayStorageBufferSize() const;

    private:
        u32 m_rayBounceRecursionDepth;
        uvec2 m_frameSize;
        IRenderer* m_renderer = nullptr;
        IRenderContext* m_context = nullptr;
        ResourceAllocator& m_resourceAllocator;

        std::unique_ptr<BVHBuilder> m_bvh;
        std::unique_ptr<BVHGeometry> m_geometryBuffer;
        BufferHandle m_bvhBuffer;
        uvec2 m_bvhTriangleOffsetRange;
        uvec2 m_bvhPrimitiveOffsetRange;
        uvec2 m_bvhMaterialOffsetRange;
        uvec2 m_bvhLightOffsetRange;
        uvec2 m_bvhNodeOffsetRange;
        uvec2 m_bvhLeafDataOffsetRange;
    };
}