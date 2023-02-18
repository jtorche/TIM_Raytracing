#pragma once
#include "rtDevice/public/IRenderer.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "resourceAllocator.h"

#include "Shaders/struct_cpp.glsl"

namespace tim
{
    class Scene;
    class SimpleCamera;
    class TextureManager;

    class RayTracingPass
    {
    public:
        struct PassResource
        {
            BufferView m_passData;
            BufferView m_tracingResult;

            BufferHandle m_reflexionRayBuffer;
            BufferHandle m_refractionRayBuffer;
        };

        RayTracingPass(IRenderer* _renderer, IRenderContext* _context, ResourceAllocator& _allocator, TextureManager& _texManager);
        ~RayTracingPass();

        void setBounceRecursionDepth(u32 _depth);
        void setFrameBufferSize(uvec2 _res);

        PassResource fillPassResources(const SimpleCamera& _camera, const Scene& _scene);
        void freePassResources(const PassResource&);

        void tracePass(ImageHandle _outputBuffer, const Scene& _scene, const PassResource& _resources);
        void lightingPass(ImageHandle _outputBuffer, const Scene& _scene, const PassResource& _resources);

    private:
        void drawBounce(const Scene& _scene, u32 _depth, BufferView _passData, BufferHandle _inputRayBuffer, ImageHandle _outputImage);
        u32 getRayStorageBufferSize() const;
        DrawArguments fillBindings(const Scene& _scene, const PassResource& _resources, PushConstants& _cst, std::vector<BufferBinding>& _bufBinds, std::vector<ImageBinding>& _imgBinds);

    private:
        IRenderer* m_renderer = nullptr;
        IRenderContext* m_context = nullptr;
        ResourceAllocator& m_resourceAllocator;
        TextureManager& m_textureManager;

        u32 m_rayBounceRecursionDepth;
        uvec2 m_frameSize;

    };
}