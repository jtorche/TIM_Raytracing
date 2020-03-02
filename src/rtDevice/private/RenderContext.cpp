#include "RenderContext.h"
#include "Common.h"
#include "VezRenderer.h"
#include "Buffer.h"
#include "Image.h"

namespace tim
{
    RenderContext::RenderContext(RenderContextType _type, u32 _queueIndex) 
        : m_contextType{ _type }, m_queueIndex{ _queueIndex } 
    {
        VezCommandBufferAllocateInfo allocInfo = {};
        allocInfo.queue = VezRenderer::get().m_graphicsQueue.m_queue;
        allocInfo.commandBufferCount = TIM_FRAME_LATENCY;
        TIM_VK_VERIFY(vezAllocateCommandBuffers(VezRenderer::get().getVkDevice(), &allocInfo, m_commandBuffer));
    }

    RenderContext::~RenderContext()
    {
        vezFreeCommandBuffers(VezRenderer::get().getVkDevice(), TIM_FRAME_LATENCY, m_commandBuffer);
    }

    void RenderContext::BeginRender()
    {
        vezBeginCommandBuffer(m_commandBuffer[VezRenderer::get().m_frameIndex], 0);
    }

    void RenderContext::EndRender()
    {
        vezEndCommandBuffer();
    }

    namespace
    {
        Image * toImage(ImageHandle _img)
        {
            return reinterpret_cast<Image*>(_img.ptr);
        }

        Buffer* toBuffer(BufferHandle _buf)
        {
            return reinterpret_cast<Buffer*>(_buf.ptr);
        }
    }

    void RenderContext::ClearBuffer(BufferHandle _buffer, u32 _data)
    {
        vezCmdFillBuffer(toBuffer(_buffer)->getVkBuffer(), 0, VK_WHOLE_SIZE, _data);
    }

    void RenderContext::ClearImage(ImageHandle _image, const Color& _color)
    {
        VezImageSubresourceRange range;
        range.baseArrayLayer = 0;
        range.baseMipLevel = 0;
        range.layerCount = VK_REMAINING_ARRAY_LAYERS;
        range.levelCount = VK_REMAINING_MIP_LEVELS;
        vezCmdClearColorImage(toImage(_image)->getVkHandle(), reinterpret_cast<const VkClearColorValue*>(&_color), 1, &range);
    }

    void RenderContext::ClearImage(ImageHandle _image, const ColorInteger& _color)
    {
        VezImageSubresourceRange range;
        range.baseArrayLayer = 0;
        range.baseMipLevel = 0;
        range.layerCount = VK_REMAINING_ARRAY_LAYERS;
        range.levelCount = VK_REMAINING_MIP_LEVELS;
        vezCmdClearColorImage(toImage(_image)->getVkHandle(), reinterpret_cast<const VkClearColorValue*>(&_color), 1, &range);
    }

    void RenderContext::Dispatch(const DrawArguments& _drawArgs, u32 _sizeX, u32 _sizeY, u32 _sizeZ)
    {
        VezPipeline pipeline = VezRenderer::get().m_psoCache->getComputePipeline(_drawArgs.m_key);
        vezCmdBindPipeline(pipeline);

        if (_drawArgs.m_constantSize > 0)
            vezCmdPushConstants(0, _drawArgs.m_constantSize, _drawArgs.m_constants);

        for (u32 i = 0; i < _drawArgs.m_numBufferBindings; ++i)
        {
            const auto& binding =_drawArgs.m_bufferBindings[i];
            const Buffer* buf = reinterpret_cast<const Buffer*>(binding.m_buffer.m_buffer.ptr);
            vezCmdBindBuffer(buf->getVkBuffer(), binding.m_buffer.m_offset, binding.m_buffer.m_range, binding.m_binding.m_set, binding.m_binding.m_slot, binding.m_binding.m_arrayElement);
        }

        for (u32 i = 0; i < _drawArgs.m_numImageBindings; ++i)
        {
            const auto& binding = _drawArgs.m_imageBindings[i];
            const Image* img = reinterpret_cast<const Image*>(binding.m_image.ptr);

            if (binding.m_sampler != SamplerType::Count)
            {
                TIM_ASSERT(binding.m_viewType == ImageViewType::Sampled);
                vezCmdBindCombinedImageSampler(img->getVkSampledView(), VezRenderer::get().m_samplers[to_integral(binding.m_sampler)], binding.m_binding.m_set, binding.m_binding.m_slot, binding.m_binding.m_arrayElement);
            }
            else
            {
                vezCmdBindImageView(binding.m_viewType == ImageViewType::Sampled ? img->getVkSampledView() : img->getVkStorageView(),
                                    VK_NULL_HANDLE, binding.m_binding.m_set, binding.m_binding.m_slot, binding.m_binding.m_arrayElement);
            }
        }

        vezCmdDispatch(_sizeX, _sizeY, _sizeZ);
    }
}