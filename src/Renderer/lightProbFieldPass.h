#pragma once
#include "rtDevice/public/IRenderer.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "resourceAllocator.h"
#include "LightProbField.h"
#include <random>

#include "Shaders/lightprob/lightprob.glsl"

namespace tim
{
    class Scene;
    class TextureManager;

    class LightProbFieldPass
    {
    public:
        LightProbFieldPass(IRenderer* _renderer, IRenderContext* _context, ResourceAllocator& _allocator, TextureManager& _texManager);
        ~LightProbFieldPass();

        void updateLightProbField(const Scene& _scene);

    private:
        IRenderer* m_renderer = nullptr;
        IRenderContext* m_context = nullptr;
        ResourceAllocator& m_resourceAllocator;
        TextureManager& m_textureManager;

    private:
        std::random_device m_rand;

        void sampleRays(vec4 rays[], SH9 shCoef[], u32 _count);
        BufferView generateIrradiance(const BufferView& _lpfConstants, const Scene& _scene);
    };
}