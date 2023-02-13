#include "TextureManager.h"
#include "Shaders/bvh/bvhBindings_cpp.glsl"

#include <FreeImage.h>

namespace tim
{
    TextureManager::TextureManager(IRenderer* _renderer, u32 _downscaleFactor) : m_renderer{ _renderer }, m_downscaleFactor{ _downscaleFactor }
    {
        ImageCreateInfo creationInfo(ImageFormat::RGBA8, 8, 8, 1, 1, ImageType::Image2D, MemoryType::Default, ImageUsage::Sampled | ImageUsage::Transfer);
        m_defaultTexture = m_renderer->CreateImage(creationInfo);

        for (u32 i = 0; i < TEXTURE_ARRAY_SIZE; ++i)
            m_samplingMode[i] = SamplerType::Repeat_Linear_MipNearest;
    }

    TextureManager::~TextureManager()
    {
        m_renderer->DestroyImage(m_defaultTexture);
        for (u32 i = 0; i < TEXTURE_ARRAY_SIZE; ++i)
        {
            if (m_images[i].ptr != 0)
                m_renderer->DestroyImage(m_images[i]);
        }
    }

    u16 TextureManager::loadTexture(const std::string& _path)
    {
        auto it = m_texPathToId.find(_path);
        if (it != m_texPathToId.end())
            return it->second;

        auto imgExt = FreeImage_GetFileType(_path.c_str());
        if (imgExt == FREE_IMAGE_FORMAT::FIF_UNKNOWN)
            return u16(-1);

        FIBITMAP* img = FreeImage_Load(imgExt, _path.c_str());
        if (!img)
            return u16(-1);

        u32 w = FreeImage_GetWidth(img);
        u32 h = FreeImage_GetHeight(img);

        if (m_downscaleFactor > 1 && w % m_downscaleFactor == 0 && h % m_downscaleFactor == 0)
        {
            w = w / m_downscaleFactor;
            h = h / m_downscaleFactor;
            FIBITMAP* prev_img = img;
            img = FreeImage_Rescale(prev_img, w, h, FILTER_BILINEAR);
        }

        u32 bpp = FreeImage_GetBPP(img);

        if (bpp < 32)
        {
            FIBITMAP* imgTmp = FreeImage_ConvertTo32Bits(img);
            FreeImage_Unload(img);
            img = imgTmp;
            bpp = FreeImage_GetBPP(img);
        }

        BYTE * data = FreeImage_GetBits(img);
        auto maskR = FreeImage_GetRedMask(img);
        auto maskG = FreeImage_GetGreenMask(img);
        auto maskB = FreeImage_GetBlueMask(img);

        ImageFormat format = ImageFormat::RGBA8_SRGB;
        if (maskR == 0x000000FF && maskG == 0x0000FF00 && maskB == 0x00FF0000)
            format = ImageFormat::RGBA8_SRGB;
        else if (maskB == 0x000000FF && maskG == 0x0000FF00 && maskR == 0x00FF0000)
            format = ImageFormat::BGRA8_SRGB;
        else
            TIM_ASSERT(false);

        ImageCreateInfo creationInfo(format, w, h, 1, 1, ImageType::Image2D, MemoryType::Default, ImageUsage::Sampled | ImageUsage::Transfer);

        u32 freeSlot = u32(-1);
        for (u32 i = 0; i < TEXTURE_ARRAY_SIZE; ++i)
        {
            if (m_images[i].ptr == 0)
            {
                freeSlot = i;
                break;
            }
        }

        TIM_ASSERT(freeSlot != u32(-1));
        m_images[freeSlot] = m_renderer->CreateImage(creationInfo);

        u32 pitch = FreeImage_GetPitch(img) / 4;
        m_renderer->UploadImage(m_images[freeSlot], data, pitch, 0);

        FreeImage_Unload(img);

        m_texPathToId[_path] = freeSlot;
        return freeSlot;
    }

    void TextureManager::setSamplingMode(u32 _index, SamplerType _mode)
    {
        m_samplingMode[_index] = _mode;
    }

    void TextureManager::fillImageBindings(std::vector<ImageBinding>& _bindings) const
    {
        for (u32 i = 0; i < TEXTURE_ARRAY_SIZE; ++i)
        {
            if (m_images[i].ptr)
            {
                ImageBinding binding;
                binding.m_binding = { 0, g_dataTextures_bind, i };
                binding.m_viewType = ImageViewType::Sampled;
                binding.m_image = m_images[i];
                binding.m_sampler = m_samplingMode[i];
                _bindings.push_back(binding);
            }
            else
            {
                ImageBinding binding;
                binding.m_binding = { 0, g_dataTextures_bind, i };
                binding.m_viewType = ImageViewType::Sampled;
                binding.m_image = m_defaultTexture;
                binding.m_sampler = m_samplingMode[i];
                _bindings.push_back(binding);
            }
        }
    }
}