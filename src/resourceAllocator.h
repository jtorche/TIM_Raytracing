#pragma once
#include "rtRenderer/public/IRenderer.h"

class ResourceAllocator
{
public:
    ResourceAllocator(tim::IRenderer* _renderer);
    ~ResourceAllocator();

    void clear();

    tim::ImageHandle allocTexture(const tim::ImageCreateInfo&);
    void releaseTexture(tim::ImageHandle);

private:
    tim::IRenderer * m_renderer = nullptr;

    struct ImageEntry
    {
        ImageEntry(const tim::ImageCreateInfo& _createInfo) : createInfo{ _createInfo } {}

        tim::ImageCreateInfo createInfo;
        tim::ImageHandle handle;
        bool isFree = true;
    };
    std::vector<ImageEntry> m_textureEntries;
};