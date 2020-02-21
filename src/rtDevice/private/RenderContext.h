#pragma once
#include "rtDevice/public/IRenderContext.h"
#include "Common.h"
#include <VEZ.h>

namespace tim
{
    class RenderContext : public IRenderContext
    {
    public:
        RenderContext(RenderContextType _type, u32 _queueIndex);
        ~RenderContext();

        void BeginRender() override;
        void EndRender() override;

        void ClearBuffer(BufferHandle _buffer, u32 _data);
        void ClearImage(ImageHandle _image, const Color& _color) override;
        void ClearImage(ImageHandle _image, const ColorInteger& _color) override;

        void Dispatch(const DrawArguments&, u32 _sizeX, u32 _sizeY, u32 _sizeZ = 1) override;

    private:
        RenderContextType m_contextType;
        u32 m_queueIndex;
        VkCommandBuffer m_commandBuffer[TIM_FRAME_LATENCY] = { VK_NULL_HANDLE };

        friend class VezRenderer;
    };
}