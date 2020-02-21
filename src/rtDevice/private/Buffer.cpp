#include "Buffer.h"
#include "VezRenderer.h"
#include "Common.h"

namespace tim
{
    Buffer::Buffer(u64 _size, MemoryType _memType, BufferUsage _usage)
    {
        VezBufferCreateInfo createInfo = {};
        createInfo.size = _size;

        if (u32(_usage) & u32(BufferUsage::ConstantBuffer))
            createInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        if (u32(_usage) & u32(BufferUsage::Storage))
            createInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        if (u32(_usage) & u32(BufferUsage::Transfer))
            createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        
        VezMemoryFlags memFlags;
        switch (_memType)
        {
        case MemoryType::Default:
            memFlags = VEZ_MEMORY_GPU_ONLY; break;
        case MemoryType::Staging:
            memFlags = VEZ_MEMORY_CPU_TO_GPU; break;
        default:
            TIM_ASSERT(false);
        }

        TIM_VK_VERIFY(vezCreateBuffer(VezRenderer::get().getVkDevice(), memFlags, &createInfo, &m_buffer));
    }

    Buffer::~Buffer()
    {
        vezDestroyBuffer(VezRenderer::get().getVkDevice(), m_buffer);
    }
}