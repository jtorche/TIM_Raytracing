#include "VezRenderer.h"
#include "Common.h"
#include "Buffer.h"
#include "Image.h"
#include "RenderContext.h"
#include "ShaderCompiler/ShaderCompiler.h"

#include <windows.h>
#include <vector>

namespace tim
{
    static const u64 g_scratchBufferSize = 4 * 1024 * 1024;
    VezRenderer* VezRenderer::s_Renderer = nullptr;

	IRenderer* createRenderer()
	{
		return new VezRenderer;
	}

	void destroyRenderer(IRenderer* _renderer)
	{
		delete _renderer;
	}

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) 
    {

        std::cerr << pCallbackData->pMessage << std::endl;

        if (IsDebuggerPresent())
        {
            OutputDebugString(pCallbackData->pMessage);
            OutputDebugString("\n");
            __debugbreak();
        }

        return VK_FALSE;
    }

	void VezRenderer::Init(ShaderCompiler& _shaderCompiler, void* _windowHandle, u32 _x, u32 _y, bool _fullscreen)
	{
        m_shaderCompiler = &_shaderCompiler;
        m_resolutionX = _x;
        m_resolutionY = _y;

        {
            VezApplicationInfo appInfo = {};
            appInfo.pApplicationName = "RayTracing";
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "TIM";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

            VezInstanceCreateInfo createInfo = {};
            createInfo.pApplicationInfo = &appInfo;
            std::vector<const char*> enabledExtensions =
            { 
                VK_KHR_SURFACE_EXTENSION_NAME, 
                VK_KHR_WIN32_SURFACE_EXTENSION_NAME, 
            #ifdef _DEBUG
                VK_EXT_DEBUG_UTILS_EXTENSION_NAME 
            #endif          
            };
            std::vector<const char*> enabledLayers = 
            { 
            #ifdef _DEBUG
                "VK_LAYER_LUNARG_standard_validation" 
            #endif
            };

            createInfo.ppEnabledLayerNames = enabledLayers.data();
            createInfo.enabledLayerCount = (u32)enabledLayers.size();
            createInfo.ppEnabledExtensionNames = enabledExtensions.data();
            createInfo.enabledExtensionCount = (u32)enabledExtensions.size();

            TIM_VK_VERIFY(vezCreateInstance(&createInfo, &m_vkInstance));
        }

        #ifdef _DEBUG
        {
            VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            createInfo.pfnUserCallback = debugCallback;
            createInfo.pUserData = nullptr; 

            auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vkInstance, "vkCreateDebugUtilsMessengerEXT");
            TIM_VK_VERIFY(func(m_vkInstance, &createInfo, nullptr, &m_debugMessenger));
        }
        #endif

        {
            VkWin32SurfaceCreateInfoKHR createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            createInfo.hwnd = *reinterpret_cast<HWND*>(_windowHandle);
            createInfo.hinstance = GetModuleHandle(nullptr);

            TIM_VK_VERIFY(vkCreateWin32SurfaceKHR(m_vkInstance, &createInfo, nullptr, &m_vkSurface));
        }

        {
            uint32_t physicalDeviceCount;
            vezEnumeratePhysicalDevices(m_vkInstance, &physicalDeviceCount, nullptr);

            std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
            vezEnumeratePhysicalDevices(m_vkInstance, &physicalDeviceCount, physicalDevices.data());

            for (auto physicalDevice : physicalDevices)
            {
                VkPhysicalDeviceProperties properties;
                vezGetPhysicalDeviceProperties(physicalDevice, &properties);

                // VkPhysicalDeviceFeatures features;
                // vezGetPhysicalDeviceFeatures(physicalDevice, &features);

                if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                {
                    VezDeviceCreateInfo deviceCreateInfo = {};
                    const char* enabledExtensions[] =
                    {
                        // VK_KHR_SURFACE_EXTENSION_NAME, 
                        VK_KHR_SWAPCHAIN_EXTENSION_NAME
                    };
                    deviceCreateInfo.enabledExtensionCount = _countof(enabledExtensions);
                    deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions;

                    m_vkPhysicalDevice = physicalDevice;
                    vezGetPhysicalDeviceProperties(m_vkPhysicalDevice, &m_physicalDeviceProperties);

                    TIM_VK_VERIFY(vezCreateDevice(physicalDevice, &deviceCreateInfo, &m_vkDevice));

                    std::cout << "Initializing on " << properties.deviceName << std::endl;
                    break;
                }

                TIM_ASSERT(m_vkDevice != VK_NULL_HANDLE);
            }
        }

        {
            //u32 queueCount;
            //VkQueueFamilyProperties queueProperties[3];
            //vezGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &queueCount, queueProperties);

            vezGetDeviceGraphicsQueue(m_vkDevice, 0, &m_graphicsQueue.m_queue);
        }

        createSwapChain(m_resolutionX, m_resolutionY);

        {
            for (u32 i = 0; i < TIM_FRAME_LATENCY; ++i)
            {
                m_scratchBuffer[i] = new Buffer(g_scratchBufferSize, MemoryType::Staging, BufferUsage::ConstantBuffer);
                void* ptr; 
                vezMapBuffer(m_vkDevice, m_scratchBuffer[i]->getVkBuffer(), 0, VK_WHOLE_SIZE, &ptr);
                TIM_ASSERT(ptr);
                m_scratchBufferCpu[i] = (ubyte *)ptr;
            }
        }

        m_psoCache = std::make_unique<PipelineStateCache>(*m_shaderCompiler);
	}

    void VezRenderer::createSwapChain(u32 _x, u32 _y)
    {
        {
            u32 numFormat;
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, m_vkSurface, &numFormat, nullptr);
            std::vector<VkSurfaceFormatKHR> supportedFormats(numFormat);
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, m_vkSurface, &numFormat, &supportedFormats[0]);

            VezSwapchainCreateInfo swapchainCreateInfo = {};
            swapchainCreateInfo.surface = m_vkSurface;
            swapchainCreateInfo.format = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
            TIM_VK_VERIFY(vezCreateSwapchain(m_vkDevice, &swapchainCreateInfo, &m_vezSwapChain));
        }

        {
            ImageCreateInfo imgCreateInfo = ImageCreateInfo(ImageFormat::BGRA8_SRGB, _x, _y);
            imgCreateInfo.usage = ImageUsage::Transfer | ImageUsage::Storage;

            for (u32 i = 0; i < TIM_FRAME_LATENCY; ++i)
                m_backBuffer[i] = new Image(imgCreateInfo);
        }

        createSamplers();
    }

    void VezRenderer::destroySwapChain()
    {
        for (u32 i = 0; i < TIM_FRAME_LATENCY; ++i)
        {
            delete m_backBuffer[i];
            m_backBuffer[i] = nullptr;
        }

        vezDestroySwapchain(m_vkDevice, m_vezSwapChain);
        m_vezSwapChain = VK_NULL_HANDLE;
    }

	void VezRenderer::Deinit()
	{
        destroySamplers();
        vezDeviceWaitIdle(m_vkDevice);

        m_psoCache.reset();

        for (u32 i = 0; i < TIM_FRAME_LATENCY; ++i)
        {
            vezUnmapBuffer(m_vkDevice, m_scratchBuffer[i]->getVkBuffer());
            delete m_scratchBuffer[i];
        }

        destroySwapChain();

        for (auto* context : m_allRenderContext)
            delete context;

        for(u32 i=0 ; i<TIM_FRAME_LATENCY ; ++i)
            vezDestroyFence(m_vkDevice, m_graphicsQueue.m_fences[i]);

        vezDestroyDevice(m_vkDevice);
        {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vkInstance, "vkDestroyDebugUtilsMessengerEXT");
            if(func)
                func(m_vkInstance, m_debugMessenger, nullptr);
        }
        vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);

        vezDestroyInstance(m_vkInstance);
	}

    void VezRenderer::Resize(u32 _x, u32 _y)
    {
        vezDeviceWaitIdle(m_vkDevice);
        destroySwapChain();
        m_resolutionX = _x;
        m_resolutionY = _y;
        createSwapChain(_x, _y);
    }

    void VezRenderer::InvalidateShaders()
    {
        vezDeviceWaitIdle(m_vkDevice);
        m_psoCache->clear();
        m_shaderCompiler->clearCache();
    }

    void VezRenderer::WaitForIdle()
    {
        vezDeviceWaitIdle(m_vkDevice);
    }

    void VezRenderer::BeginFrame()
    {
        if (m_graphicsQueue.m_fences[m_frameIndex] != VK_NULL_HANDLE)
        {
            TIM_VK_VERIFY(vezWaitForFences(m_vkDevice, 1, &m_graphicsQueue.m_fences[m_frameIndex], true, u64(-1)));
        }
    }

    void VezRenderer::EndFrame()
    {
        VkDeviceSize flushAlignment = m_physicalDeviceProperties.limits.nonCoherentAtomSize;
        VezMappedBufferRange range = { m_scratchBuffer[m_frameIndex]->getVkBuffer(), 0, alignUp(m_scratchBufferCursor.load(), flushAlignment) };
        if(range.size > 0)
            vezFlushMappedBufferRanges(m_vkDevice, 1, &range);

        VezSubmitInfo submitInfo = {};
        submitInfo.commandBufferCount = (u32)m_graphicsQueue.m_cmdToSubmit.size();
        submitInfo.pCommandBuffers = m_graphicsQueue.m_cmdToSubmit.data();
        
        if (m_graphicsQueue.m_backBufferReady[m_frameIndex] != VK_NULL_HANDLE)
        {
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &m_graphicsQueue.m_backBufferReady[m_frameIndex];
            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
            submitInfo.pWaitDstStageMask = waitStages;
        }

        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &m_graphicsQueue.m_renderingFinished[m_frameIndex];

        vezQueueSubmit(m_graphicsQueue.m_queue, 1, &submitInfo, &m_graphicsQueue.m_fences[m_frameIndex]);

        m_graphicsQueue.m_cmdToSubmit.clear();

        m_scratchBufferCursor = 0;
        m_frameIndex = (m_frameIndex + 1) % TIM_FRAME_LATENCY;
    }

    void VezRenderer::Execute(IRenderContext* _context)
    {
        RenderContext* context = reinterpret_cast<RenderContext*>(_context);
        TIM_ASSERT(context->m_contextType == RenderContextType::Graphics);

        m_graphicsQueue.m_cmdToSubmit.push_back(context->m_commandBuffer[m_frameIndex]);
    }

    void VezRenderer::Present()
    {
        if (m_graphicsQueue.m_renderingFinished[m_frameIndex] != VK_NULL_HANDLE)
        {
            VezPresentInfo presentInfo = {};
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &m_vezSwapChain;

            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &m_graphicsQueue.m_renderingFinished[m_frameIndex];
            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
            presentInfo.pWaitDstStageMask = waitStages;

            presentInfo.signalSemaphoreCount = 1;
            presentInfo.pSignalSemaphores = &m_graphicsQueue.m_backBufferReady[m_frameIndex];
            
            VkImage imgToPresent[] = { m_backBuffer[m_frameIndex]->getVkHandle() };
            presentInfo.pImages = imgToPresent;

            TIM_VK_VERIFY(vezQueuePresent(m_graphicsQueue.m_queue, &presentInfo));
        }
    }

    ImageHandle VezRenderer::GetBackBuffer() const
    {
        return ImageHandle{ m_backBuffer[m_frameIndex] };
    }

    BufferHandle VezRenderer::CreateBuffer(u32 _size, MemoryType _memType, BufferUsage _usage)
    {
        return BufferHandle{ reinterpret_cast<void *>(new Buffer(_size, _memType, _usage)) };
    }

    void VezRenderer::DestroyBuffer(BufferHandle& _buffer)
    {
        Buffer* buf = reinterpret_cast<Buffer *>(_buffer.ptr);
        delete buf;
        _buffer.ptr = nullptr;
    }

    void VezRenderer::UploadBuffer(BufferHandle _handle, void* _data, u32 _dataSize)
    {
        Buffer* buf = reinterpret_cast<Buffer*>(_handle.ptr);
        vezBufferSubData(m_vkDevice, buf->getVkBuffer(), 0, _dataSize, _data);
    }

    void VezRenderer::UploadBuffer(BufferHandle _handle, u32 _destOffset, void* _data, u32 _dataSize)
    {
		Buffer* buf = reinterpret_cast<Buffer*>(_handle.ptr);
		vezBufferSubData(m_vkDevice, buf->getVkBuffer(), _destOffset, _dataSize, _data);
    }

    void VezRenderer::UploadImage(ImageHandle _handle, void* _data, u32 _pitch)
    {
        Image* img = reinterpret_cast<Image*>(_handle.ptr);
        VezImageSubDataInfo info = {};
        info.dataRowLength = _pitch;
        info.imageExtent = { img->getDesc().width, img->getDesc().height, 1 };
        info.imageOffset = { 0,0,0 };
        info.imageSubresource = { 0,0,1 };
        vezImageSubData(m_vkDevice, img->getVkHandle(), &info, _data);
    }

    ubyte * VezRenderer::GetDynamicBuffer(u32 _size, BufferView& _buffer)
    {
        u64 offset = m_scratchBufferCursor.fetch_add(_size);
        TIM_ASSERT(offset + _size <= g_scratchBufferSize);

        _buffer.m_buffer = { m_scratchBuffer[m_frameIndex] };
        _buffer.m_offset = (u32)offset;
        _buffer.m_range = _size;

        return m_scratchBufferCpu[m_frameIndex] + offset;
    }

    IRenderContext * VezRenderer::CreateRenderContext(RenderContextType _type, u32 _queueIndex)
    {
        m_allRenderContext.push_back(new RenderContext(_type, _queueIndex));
        return m_allRenderContext.back();
    }

    ImageHandle VezRenderer::CreateImage(const ImageCreateInfo& _info)
    {
        Image * image = new Image(_info);
        return ImageHandle{ reinterpret_cast<Image*>(image) };
    }

    void VezRenderer::DestroyImage(ImageHandle& _image)
    {
        Image* image = reinterpret_cast<Image *>(_image.ptr);
        delete image;
        _image.ptr = nullptr;
    }

    void VezRenderer::createSamplers()
    {
        VezSamplerCreateInfo info = {};

        // Clamp_Nearest_MipNearest
        info.magFilter = VkFilter::VK_FILTER_NEAREST;
        info.minFilter = VkFilter::VK_FILTER_NEAREST;
        info.mipmapMode = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST;
        info.addressModeU = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.addressModeV = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.addressModeW = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        vezCreateSampler(m_vkDevice, &info, &m_samplers[to_integral(SamplerType::Clamp_Nearest_MipNearest)]);

        // Repeat_Nearest_MipNearest
        info.magFilter = VkFilter::VK_FILTER_NEAREST;
        info.minFilter = VkFilter::VK_FILTER_NEAREST;
        info.mipmapMode = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST;
        info.addressModeU = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeV = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeW = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vezCreateSampler(m_vkDevice, &info, &m_samplers[to_integral(SamplerType::Repeat_Nearest_MipNearest)]);

        // Clamp_Linear_MipNearest
        info.magFilter = VkFilter::VK_FILTER_LINEAR;
        info.minFilter = VkFilter::VK_FILTER_LINEAR;
        info.mipmapMode = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST;
        info.addressModeU = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.addressModeV = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.addressModeW = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        vezCreateSampler(m_vkDevice, &info, &m_samplers[to_integral(SamplerType::Clamp_Linear_MipNearest)]);

        // Repeat_Linear_MipNearest
        info.magFilter = VkFilter::VK_FILTER_LINEAR;
        info.minFilter = VkFilter::VK_FILTER_LINEAR;
        info.mipmapMode = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST;
        info.addressModeU = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeV = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeW = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vezCreateSampler(m_vkDevice, &info, &m_samplers[to_integral(SamplerType::Repeat_Linear_MipNearest)]);
    }

    void VezRenderer::destroySamplers()
    {
        for (u32 i = 0; i < to_integral(SamplerType::Count); ++i)
        {
            vezDestroySampler(m_vkDevice, m_samplers[i]);
            m_samplers[i] = {};
        }
    }
}