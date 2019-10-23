#pragma once
#include <inttypes.h>
#include "linalg.h"

namespace tim
{
    using u16 = uint16_t;
	using u32 = uint32_t;
    using uint = uint32_t;
	using u64 = uint64_t;

	using i32 = int32_t;
	using i64 = int64_t;

    using ubyte = unsigned char;

    struct Color
    {
        float r, g, b, a;
    };

    struct ColorInteger
    {
        union
        {
            struct { ubyte r, g, b, a; };
            u32 color;
        };
    };

    using vec4 = linalg::aliases::float4;
    using ivec4 = linalg::aliases::int4;
    using vec3 = linalg::aliases::float3;
    using vec2 = linalg::aliases::float2;
    using mat4 = linalg::aliases::float4x4;
    using uvec2 = linalg::aliases::uint2;
    using uvec4 = linalg::aliases::uint4;

    template<typename T>
    constexpr T alignUp(T x, T align)
    {
        return x + (x % align == 0 ? 0 : align - x % align);
    }
}