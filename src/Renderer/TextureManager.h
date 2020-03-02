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
        void fillImageBindings(std::vector<ImageBinding>& _bindings) const;

    private:
        IRenderer * m_renderer;
        ImageHandle m_images[TEXTURE_ARRAY_SIZE];
    };
}