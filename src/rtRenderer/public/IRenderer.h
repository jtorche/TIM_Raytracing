#include "timCore/type.h"
#include "IRenderContext.h"
#include "Resource.h"
#include <vulkan/vulkan.h>

namespace tim
{
    class ShaderCompiler;

	class IRenderer
	{
	public:
        virtual ~IRenderer() {}

		virtual void Init(ShaderCompiler&, void* _windowHandle, u32 _x, u32 _y, bool _fullscreen) = 0;
		virtual void Deinit() = 0;
        virtual void Resize(u32 _x, u32 _y) = 0;

        virtual void InvalidateShaders() = 0;

        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;
        virtual void Execute(IRenderContext *) = 0;
        virtual void Present() = 0;

        virtual ImageHandle GetBackBuffer() const = 0;

        virtual BufferHandle CreateBuffer(u32 _size, MemoryType _memType, BufferUsage _usage) = 0;
        virtual void DestroyBuffer(BufferHandle& _buffer) = 0;
        virtual void UploadBuffer(BufferHandle _handle, void * _data, u32 _dataSize) = 0;

        virtual ubyte * GetDynamicBuffer(u32 _size, BufferView& _buffer) = 0;

        virtual ImageHandle CreateImage(const ImageCreateInfo& _info) = 0;
        virtual void DestroyImage(ImageHandle& _buffer) = 0;

        virtual IRenderContext * CreateRenderContext(RenderContextType, u32 _queueIndex = 0) = 0;
    };

    IRenderer* createRenderer();
	void destroyRenderer(IRenderer* _renderer);
}