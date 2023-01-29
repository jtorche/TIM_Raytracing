#pragma once
#include "rtDevice/public/IRenderer.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "resourceAllocator.h"
#include <random>

namespace tim
{
    class Scene;
    class TextureManager;

    class LightProbFieldPass
    {
    public:
        LightProbFieldPass(IRenderer* _renderer, IRenderContext* _context, ResourceAllocator& _allocator, TextureManager& _texManager);
        ~LightProbFieldPass() = default;

        void generateLPF(const Scene& _scene);

    private:
        IRenderer* m_renderer = nullptr;
        IRenderContext* m_context = nullptr;
        ResourceAllocator& m_resourceAllocator;
        TextureManager& m_textureManager;

    private:
        std::random_device m_rand;
        uvec3 m_fieldSize;

        void sampleRays(vec4 rays[], u32 _count);
    };
}