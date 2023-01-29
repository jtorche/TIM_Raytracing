#pragma once
#include "rtDevice/public/IRenderer.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "resourceAllocator.h"

namespace tim
{
    class Scene;
    class SimpleCamera;
    class TextureManager;

    class RayTracingPass
    {
    public:
        RayTracingPass(IRenderer* _renderer, IRenderContext* _context, ResourceAllocator& _allocator, TextureManager& _texManager);
        ~RayTracingPass();

        void setBounceRecursionDepth(u32 _depth);
        void setFrameBufferSize(uvec2 _res);
        void draw(ImageHandle _outputBuffer, const Scene& _scene, const SimpleCamera& _camera);

    private:
        void drawBounce(const Scene& _scene, u32 _depth, BufferView _passData, BufferHandle _inputRayBuffer, ImageHandle _outputImage);
        u32 getRayStorageBufferSize() const;

    private:
        IRenderer* m_renderer = nullptr;
        IRenderContext* m_context = nullptr;
        ResourceAllocator& m_resourceAllocator;
        TextureManager& m_textureManager;

        u32 m_rayBounceRecursionDepth;
        uvec2 m_frameSize;

    };
}