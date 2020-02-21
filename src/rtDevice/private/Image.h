#pragma once
#include <VEZ.h>
#include "rtDevice/public/Resource.h"

namespace tim
{
    class Image
    {
    public:
        Image(const ImageCreateInfo&);
        ~Image();

        const ImageCreateInfo& getDesc() const { return m_desc; }

        VkImage getVkHandle() const { return m_image; }
        VkImageView getVkSampledView() const { return m_sampledView; }
        VkImageView getVkStorageView() const { return m_storageView; }

    private:
        ImageCreateInfo m_desc;
        VkImage m_image;
        VkImageView m_storageView = VK_NULL_HANDLE;
        VkImageView m_sampledView = VK_NULL_HANDLE;
    };
}
