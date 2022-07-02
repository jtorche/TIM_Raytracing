#pragma once

#include "timCore/Common.h"
#include "rtDevice/public/IRenderer.h"
#include "Shaders/struct_cpp.glsl"

namespace tim
{
    class TextureManager
    {
    public: 
        TextureManager(IRenderer * _renderer);
        ~TextureManager();

        u16 loadTexture(const char* _path);
        void setSamplingMode(u32 _index, SamplerType _mode);
        void fillImageBindings(std::vector<ImageBinding>& _bindings) const;

        void unloadAllImages();

    private:
        IRenderer * m_renderer;
        ImageHandle m_images[TEXTURE_ARRAY_SIZE];
        SamplerType m_samplingMode[TEXTURE_ARRAY_SIZE];
        ImageHandle m_defaultTexture;
    };
}