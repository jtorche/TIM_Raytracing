#pragma once
#include "rtDevice/public/IRenderer.h"

namespace tim
{
    class ResourceAllocator
    {
    public:
        ResourceAllocator(IRenderer* _renderer);
        ~ResourceAllocator();

        void clear();

        ImageHandle allocTexture(const ImageCreateInfo&);
        void releaseTexture(ImageHandle);

        BufferHandle allocBuffer(u32 _size, BufferUsage _usage, MemoryType _memType = MemoryType::Default);
        void releaseBuffer(BufferHandle);

    private:
        IRenderer* m_renderer = nullptr;

        struct ImageEntry
        {
            ImageEntry(const ImageCreateInfo& _createInfo) : createInfo{ _createInfo } {}

            ImageCreateInfo createInfo;
            ImageHandle handle;
            bool isFree = true;
        };
        std::vector<ImageEntry> m_textureEntries;

        struct BufferCreateInfo
        {
            u32 m_size;
            MemoryType m_memType;
            BufferUsage m_usage;

            bool operator==(const BufferCreateInfo&) const = default;
        };

        struct BufferEntry
        {
            BufferCreateInfo createInfo;
            BufferHandle handle;
            bool isFree = true;
        };
        std::vector<BufferEntry> m_bufferEntries;
    };
}