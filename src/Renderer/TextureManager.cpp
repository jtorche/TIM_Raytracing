#include "TextureManager.h"

#include <FreeImage.h>

namespace tim
{
    TextureManager::TextureManager(IRenderer* _renderer) : m_renderer{ _renderer }
    {

    }

    TextureManager::~TextureManager()
    {
        for (u32 i = 0; i < TEXTURE_ARRAY_SIZE; ++i)
        {
            m_renderer->DestroyImage(m_images[i]);
        }
    }

    u16 TextureManager::loadTexture(const char* _path)
    {
        auto imgExt = FreeImage_GetFileType(_path);
        if (imgExt == FREE_IMAGE_FORMAT::FIF_UNKNOWN)
            return u16(-1);

        FIBITMAP* img = FreeImage_Load(imgExt, _path);
        if (!img)
            return u16(-1);

        u32 w = FreeImage_GetWidth(img);
        u32 h = FreeImage_GetHeight(img);
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

        ImageFormat format = ImageFormat::RGBA8;
        ImageCreateInfo creationInfo(format, w, h, 1, ImageType::Image2D, MemoryType::Default, ImageUsage::Sampled | ImageUsage::Transfer);

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
        m_renderer->UploadImage(m_images[freeSlot], data, pitch);

        FreeImage_Unload(img);
        return freeSlot;
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
                binding.m_sampler = SamplerType::Repeat_Linear_MipNearest;
                _bindings.push_back(binding);
            }
        }
    }
}