#include "Image.h"
#include "Common.h"
#include"VezRenderer.h"

namespace tim
{
    namespace
    {
        VkFormat toVkFormat(ImageFormat _format, bool _isStorage)
        {
            switch (_format)
            {
            case ImageFormat::BGRA8_SRGB:
                return _isStorage ? VkFormat::VK_FORMAT_B8G8R8A8_UNORM : VkFormat::VK_FORMAT_B8G8R8A8_SRGB;
            case ImageFormat::RGBA16F:
                return VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;
            case ImageFormat::RGBA8:
                return VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
            case ImageFormat::BGRA8:
                return VkFormat::VK_FORMAT_B8G8R8A8_UNORM;
            case ImageFormat::RGBA8_SRGB:
                return _isStorage ? VkFormat::VK_FORMAT_R8G8B8A8_UNORM : VkFormat::VK_FORMAT_R8G8B8A8_SRGB;
            default:
                TIM_ASSERT(false);
                return VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
            }
        }
    }

    Image::Image(const ImageCreateInfo& _desc) : m_desc{ _desc }
    {
        VezImageCreateInfo info = {};
        VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
        switch (m_desc.type)
        {
        case ImageType::Image1D:
            info.imageType = VkImageType::VK_IMAGE_TYPE_1D;
            viewType = VK_IMAGE_VIEW_TYPE_1D;
            break;
        case ImageType::Image2D:
            info.imageType = VkImageType::VK_IMAGE_TYPE_2D;
            viewType = VK_IMAGE_VIEW_TYPE_2D;
            break;
        case ImageType::Image2D_Array:
            info.imageType = VkImageType::VK_IMAGE_TYPE_2D;
            info.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
            viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            break;
        case ImageType::ImageCube:
            info.imageType = VkImageType::VK_IMAGE_TYPE_2D;
            info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            break;
        case ImageType::Image3D:
            info.imageType = VkImageType::VK_IMAGE_TYPE_3D;
            viewType = VK_IMAGE_VIEW_TYPE_3D;
            break;
        default:
            TIM_ASSERT(false);
        }

        bool isStorage = false;
        bool isSampled = false;

        if (asBool(m_desc.usage & ImageUsage::RenderTarget))
            info.usage |= VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (asBool(m_desc.usage & ImageUsage::Sampled))
        {
            info.usage |= VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;
            isSampled = true;
        }
        if (asBool(m_desc.usage & ImageUsage::Storage))
        {
            info.usage |= VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT;
            isStorage = true;
        }
        if (asBool(m_desc.usage & ImageUsage::Transfer))
            info.usage |= VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        info.extent = { _desc.width, _desc.height, m_desc.type == ImageType::Image3D ? _desc.depth : 1 };
        info.arrayLayers = m_desc.type == ImageType::Image2D_Array ? _desc.depth : 1;
        info.mipLevels = 1;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.queueFamilyIndexCount = 0;

        VezMemoryFlags memFlags;
        switch (m_desc.memory)
        {
        case MemoryType::Default:
            memFlags = VEZ_MEMORY_GPU_ONLY; break;
        case MemoryType::Staging:
            memFlags = VEZ_MEMORY_CPU_TO_GPU; break;
        default:
            TIM_ASSERT(false);
        }

        info.format = toVkFormat(m_desc.format, isStorage);

        TIM_VK_VERIFY(vezCreateImage(VezRenderer::get().getVkDevice(), memFlags, &info, &m_image));

        VezImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.image = m_image;
        viewCreateInfo.viewType = viewType;
        viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.layerCount = info.arrayLayers;
        viewCreateInfo.subresourceRange.levelCount = 1;
  
        if (isStorage)
        {
            viewCreateInfo.format = toVkFormat(m_desc.format, isStorage);
            TIM_VK_VERIFY(vezCreateImageView(VezRenderer::get().getVkDevice(), &viewCreateInfo, &m_storageView));
        }
        if (isSampled)
        {
            viewCreateInfo.format = toVkFormat(m_desc.format, false);
            TIM_VK_VERIFY(vezCreateImageView(VezRenderer::get().getVkDevice(), &viewCreateInfo, &m_sampledView));
        }
        
    }

    Image::~Image()
    {
        vezDestroyImageView(VezRenderer::get().getVkDevice(), m_sampledView);
        vezDestroyImageView(VezRenderer::get().getVkDevice(), m_storageView);
        vezDestroyImage(VezRenderer::get().getVkDevice(), m_image);
    }
}