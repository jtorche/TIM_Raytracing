#include "resourceAllocator.h"
#include "timCore/Common.h"

namespace tim
{

    ResourceAllocator::ResourceAllocator(IRenderer* _renderer) : m_renderer{ _renderer }
    {

    }

    ResourceAllocator::~ResourceAllocator()
    {
        clear();
    }

    void ResourceAllocator::clear()
    {
        for (auto& entry : m_textureEntries)
        {
            TIM_ASSERT(entry.isFree);
            m_renderer->DestroyImage(entry.handle);
        }
        m_textureEntries.clear();

        for (auto& entry : m_bufferEntries)
        {
            TIM_ASSERT(entry.isFree);
            m_renderer->DestroyBuffer(entry.handle);
        }
        m_bufferEntries.clear();
    }

    ImageHandle ResourceAllocator::allocTexture(const ImageCreateInfo& _createInfo)
    {
        for (auto& entry : m_textureEntries)
        {
            if (entry.isFree && entry.createInfo == _createInfo)
            {
                entry.isFree = false;
                return entry.handle;
            }
        }

        ImageEntry entry(_createInfo);
        entry.handle = m_renderer->CreateImage(_createInfo);
        entry.isFree = false;
        m_textureEntries.push_back(entry);

        return entry.handle;
    }

    void ResourceAllocator::releaseTexture(ImageHandle _handle)
    {
        for (auto& entry : m_textureEntries)
        {
            if (_handle.ptr == entry.handle.ptr)
            {
                TIM_ASSERT(!entry.isFree);
                entry.isFree = true;
                return;
            }
        }

        TIM_ASSERT(false);
    }

    BufferHandle ResourceAllocator::allocBuffer(u32 _size, BufferUsage _usage, MemoryType _memType)
    {
        BufferCreateInfo createInfo{ _size, _memType, _usage };
        for (auto& entry : m_bufferEntries)
        {
            if (entry.isFree && entry.createInfo == createInfo)
            {
                entry.isFree = false;
                return entry.handle;
            }
        }

        BufferEntry entry;
        entry.createInfo = createInfo;
        entry.handle = m_renderer->CreateBuffer(createInfo.m_size, createInfo.m_memType, createInfo.m_usage);
        entry.isFree = false;
        m_bufferEntries.push_back(entry);

        return entry.handle;
    }

    void ResourceAllocator::releaseBuffer(BufferHandle _handle)
    {
        for (auto& entry : m_bufferEntries)
        {
            if (_handle.ptr == entry.handle.ptr)
            {
                TIM_ASSERT(!entry.isFree);
                entry.isFree = true;
                return;
            }
        }

        TIM_ASSERT(false);
    }
}