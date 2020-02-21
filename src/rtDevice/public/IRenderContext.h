#pragma once
#include "timCore/type.h"
#include "shaderCompiler/ShaderFlags.h"
#include "Resource.h"
#include <vector>

namespace tim
{
    enum class RenderContextType
    {
        Graphics,
        Compute
    };

    struct BindingPoint
    {
        u16 m_set = 0;
        u16 m_slot = 0;
        u32 m_arrayElement = 0;
    };

    enum class ImageViewType
    {
        Storage,
        Sampled
    };

    struct ImageBinding
    {
        ImageHandle m_image;
        ImageViewType m_viewType;
        BindingPoint m_binding;
    };

    struct BufferView
    {
        BufferHandle m_buffer;
        u32 m_offset;
        u32 m_range;
    };

    struct BufferBinding
    {
        BufferView m_buffer;
        BindingPoint m_binding;
    };

    struct DrawArguments
    {
        ShaderKey m_key;
        BufferBinding * m_bufferBindings = nullptr;
        u32 m_numBufferBindings = 0;
        ImageBinding * m_imageBindings = nullptr;
        u32 m_numImageBindings = 0;
        void * m_constants = nullptr;
        u32 m_constantSize = 0;
    };

	class IRenderContext
	{
	public:
        virtual void BeginRender() = 0;
        virtual void EndRender() = 0;

        virtual void ClearBuffer(BufferHandle _image, u32 _data) = 0;
        virtual void ClearImage(ImageHandle _image, const Color& _color) = 0;
        virtual void ClearImage(ImageHandle _image, const ColorInteger& _color) = 0;

        virtual void Dispatch(const DrawArguments&, u32 _sizeX, u32 _sizeY, u32 _sizeZ = 1) = 0;

    protected:
        virtual ~IRenderContext() {}
    };
}