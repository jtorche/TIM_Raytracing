#pragma once

#include "timCore/Common.h"
#include "timCore/flat_hash_map.h"
#include "rtDevice/public/IRenderer.h"
#include "Shaders/struct_cpp.glsl"

namespace tim
{
    class TextureManager
    {
    public: 
        TextureManager(IRenderer * _renderer, u32 _downscaleFactor);
        ~TextureManager();

        u16 loadTexture(const std::string& _path);
        void setSamplingMode(u32 _index, SamplerType _mode);
        void fillImageBindings(std::vector<ImageBinding>& _bindings) const;

    private:
        IRenderer * m_renderer;
        const u32 m_downscaleFactor;
        ImageHandle m_images[TEXTURE_ARRAY_SIZE];
        SamplerType m_samplingMode[TEXTURE_ARRAY_SIZE];
        ImageHandle m_defaultTexture;
        ska::flat_hash_map<std::string, u32> m_texPathToId;
    };
}