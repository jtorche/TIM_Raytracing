#pragma once
#include "timCore/type.h"
#include "timCore/Common.h"

namespace tim
{
    enum class MemoryType
    {
        Default,
        Staging,
    };
    
    enum class BufferUsage : u32
    {
        ConstantBuffer      = 0x1,
        Storage             = 0x2,
        Transfer            = 0x4,
    };

    inline BufferUsage operator|(BufferUsage a, BufferUsage b)
    {
        return BufferUsage(u32(a) | u32(b));
    }

    enum class ImageUsage : u32
    {
        RenderTarget = 0x1,
        Storage = 0x2,
        Transfer = 0x4,
        Sampled = 0x8,
    };

    inline ImageUsage operator|(ImageUsage a, ImageUsage b) 
    {
        return ImageUsage(u32(a) | u32(b));
    }

    inline ImageUsage operator&(ImageUsage a, ImageUsage b)
    {
        return ImageUsage(u32(a) & u32(b));
    }

    inline bool asBool(ImageUsage a)
    {
        return u32(a) > 0;
    }

    enum class ImageType
    {
        Image1D,
        Image2D,
        Image2D_Array,
        ImageCube,
        Image3D,
    };

    enum class ImageFormat
    {
        BGRA8_SRGB,
        RGBA8,
        BGRA8,
        RGBA8_SRGB,
        RGBA16F,
    };

    struct ImageCreateInfo
    {
        ImageCreateInfo(ImageFormat _format, u32 _width, u32 _height, u32 _depth = 1, u32 _numMips = 1, ImageType _type = ImageType::Image2D, MemoryType _mem = MemoryType::Default, ImageUsage _usage = ImageUsage::Storage) :
            type{ _type }, format{ _format }, width{ _width }, height{ _height }, depth{ _depth }, numMips{ _numMips }, memory{ _mem }, usage{ _usage } 
        {
            if (numMips == u32(-1)) // compute mip count automatically
            {
                TIM_ASSERT(isPowerOf2(_width) && isPowerOf2(_height));
                numMips = std::max(log2(_width), log2(_height));
            }
        }

        bool operator==(const ImageCreateInfo& _info) const = default;

        ImageType type;
        ImageFormat format;
        u32 width, height, depth, numMips;
        MemoryType memory;
        ImageUsage usage;
    };

    struct ImageHandle
    {
        void* ptr = nullptr;
    };

    struct BufferHandle
    {
        void* ptr = nullptr;
    };

    enum class SamplerType : u32
    {
        Clamp_Nearest_MipNearest,
        Repeat_Nearest_MipNearest,
        Clamp_Linear_MipNearest,
        Repeat_Linear_MipNearest,
        Clamp_Linear_MipLinear,
        Repeat_Linear_MipLinear,

        Count
    };
}