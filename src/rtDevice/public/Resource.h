#pragma once
#include "timCore/type.h"

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
        ImageCreateInfo(ImageFormat _format, u32 _width, u32 _height, u32 _depth = 1, ImageType _type = ImageType::Image2D, MemoryType _mem = MemoryType::Default, ImageUsage _usage = ImageUsage::Storage) :
            type{ _type }, format{ _format }, width{ _width }, height{ _height }, depth{ _depth }, memory{ _mem }, usage{ _usage } {}

        bool operator==(const ImageCreateInfo& _info) const = default;

        ImageType type;
        ImageFormat format;
        u32 width, height, depth;
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

        Count
    };
}