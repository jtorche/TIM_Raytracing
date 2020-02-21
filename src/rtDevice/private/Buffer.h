#pragma once
#include <VEZ.h>
#include "rtDevice/public/Resource.h"

namespace tim
{
    class Buffer
    {
    public:
        Buffer(u64 _size, MemoryType _memType, BufferUsage _usage);
        ~Buffer();

        VkBuffer getVkBuffer() const { return m_buffer; }

    private:
        VkBuffer m_buffer = VK_NULL_HANDLE;
    };
}