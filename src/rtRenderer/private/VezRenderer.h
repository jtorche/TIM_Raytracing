#include "rtRenderer/public/IRenderer.h"
#include "PipelineStateCache.h"
#include <VEZ.h>
#include <vector>
#include <atomic>

#include "Common.h"

namespace tim
{
    class IRenderContext;
    class Image;
    class Buffer;

	class VezRenderer : public IRenderer
	{
	public:
        static VezRenderer& get() { return *s_Renderer; }

        VezRenderer() { s_Renderer = this; }
        ~VezRenderer() { s_Renderer = nullptr; }

		void Init(ShaderCompiler& _shaderCompiler, void * _windowHandle, u32 _x, u32 _y, bool _fullscreen) override;
		void Deinit() override;
        void Resize(u32 _x, u32 _y) override;
        void InvalidateShaders() override;

        void WaitForIdle() override;
        void BeginFrame() override;
        void EndFrame() override;
        void Execute(IRenderContext *) override;
        void Present() override;

        ImageHandle GetBackBuffer() const override;

        BufferHandle CreateBuffer(u32 _size, MemoryType _memType, BufferUsage _usage) override;
        void DestroyBuffer(BufferHandle& _buffer) override;
        void UploadBuffer(BufferHandle _handle, void* _data, u32 _dataSize) override;

        ubyte* GetDynamicBuffer(u32 _size, BufferView& _buffer) override;

        ImageHandle CreateImage(const ImageCreateInfo& _info) override;
        void DestroyImage(ImageHandle& _buffer) override;

        IRenderContext * CreateRenderContext(RenderContextType _type, u32 _queueIndex = 0);

    public:
        VkDevice getVkDevice() const { return m_vkDevice; }

    private:
        friend class RenderContext;

        void createSwapChain(u32 _x, u32 _y);
        void destroySwapChain();

	private:
        static VezRenderer* s_Renderer;

        ShaderCompiler * m_shaderCompiler = nullptr;
        u32 m_resolutionX = 0, m_resolutionY = 0;
        VkInstance m_vkInstance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR m_vkSurface = VK_NULL_HANDLE;
        VkPhysicalDevice m_vkPhysicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties m_physicalDeviceProperties;
        VkDevice m_vkDevice = VK_NULL_HANDLE;
        VezSwapchain m_vezSwapChain = 0;

        Image * m_backBuffer[TIM_FRAME_LATENCY] = { { nullptr } };
        Buffer * m_scratchBuffer[TIM_FRAME_LATENCY] = { { nullptr } };
        ubyte * m_scratchBufferCpu[TIM_FRAME_LATENCY] = { { nullptr } };
        std::atomic<u64> m_scratchBufferCursor = 0;
        u32 m_frameIndex = 0;

        struct Queue
        {
            VkQueue m_queue = VK_NULL_HANDLE;
            std::vector<VkCommandBuffer> m_cmdToSubmit;

            VkSemaphore m_renderingFinished [TIM_FRAME_LATENCY] = { { VK_NULL_HANDLE } };
            VkSemaphore m_backBufferReady   [TIM_FRAME_LATENCY] = { { VK_NULL_HANDLE } };
            VkFence m_fences                [TIM_FRAME_LATENCY] = { { VK_NULL_HANDLE } };
        };
        Queue m_graphicsQueue;

        std::unique_ptr<PipelineStateCache> m_psoCache;

        std::vector<RenderContext *> m_allRenderContext;
	};
}