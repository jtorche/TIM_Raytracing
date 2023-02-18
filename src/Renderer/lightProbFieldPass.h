#pragma once
#include "rtDevice/public/IRenderer.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "resourceAllocator.h"
#include "LightProbField.h"
#include <random>

#include "Shaders/struct_cpp.glsl"
#include "Shaders/lightprob/lightprob.glsl"

namespace tim
{
    class Scene;
    class TextureManager;

    class LightProbFieldPass
    {
    public:
        struct PassResource
        {
            BufferView m_lpfConstants;
            BufferView m_shCoefsBuffer;

            BufferView m_irradianceField;
            BufferView m_tracingResult;
        };

        LightProbFieldPass(IRenderer* _renderer, IRenderContext* _context, ResourceAllocator& _allocator, TextureManager& _texManager);
        ~LightProbFieldPass();

        PassResource fillPassResources(const Scene& _scene);
        void freePassResources(const PassResource&);

        void traceLightProbField(const Scene& _scene, const PassResource& _resources);
        void updateLightProbField(const Scene& _scene, const PassResource& _resources);

    private:
        IRenderer* m_renderer = nullptr;
        IRenderContext* m_context = nullptr;
        ResourceAllocator& m_resourceAllocator;
        TextureManager& m_textureManager;

    private:
        std::random_device m_rand;

        void sampleRays(vec4 rays[], SH9 shCoef[], u32 _count);
        DrawArguments fillBindings(const Scene& _scene, const PassResource& _resources, PushConstants& _cst, std::vector<BufferBinding>& _bufBinds, std::vector<ImageBinding>& _imgBinds);
    };
}