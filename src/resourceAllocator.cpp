#include "resourceAllocator.h"
#include "timCore/Common.h"

using namespace tim;

ResourceAllocator::ResourceAllocator(tim::IRenderer* _renderer) : m_renderer{ _renderer }
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
}

tim::ImageHandle ResourceAllocator::allocTexture(const tim::ImageCreateInfo& _createInfo)
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

void ResourceAllocator::releaseTexture(tim::ImageHandle _handle)
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