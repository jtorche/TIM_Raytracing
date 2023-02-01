#pragma once
#include "rtDevice/public/IRenderer.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "resourceAllocator.h"
#include <random>

#include "Shaders/lightprob.glsl"

namespace tim
{
    class Scene;
    class TextureManager;

    struct GpuLightProbField
    {
        void allocate(IRenderer* _renderer, uvec3 _fieldSize);
        void free(IRenderer* _renderer);

        ImageHandle lightProbFieldR[2];
        ImageHandle lightProbFieldG[2];
        ImageHandle lightProbFieldB[2];
        ImageHandle lightProbFieldY00;
    };

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
        uvec3 m_fieldSize;
        GpuLightProbField m_field;

        void sampleRays(vec4 rays[], SH9 shCoef[], u32 _count);
        BufferView generateIrradiance(const BufferView& _lpfConstants, const Scene& _scene);
    };
}