#pragma once
#include <inttypes.h>
#include "linalg.h"

using u16 = uint16_t;
using u32 = uint32_t;
using uint = uint32_t;
using u64 = uint64_t;

using i32 = int32_t;
using i64 = int64_t;

using ubyte = unsigned char;

using vec4 = linalg::aliases::float4;
using ivec4 = linalg::aliases::int4;
using vec3 = linalg::aliases::float3;
using vec2 = linalg::aliases::float2;
using mat4 = linalg::aliases::float4x4;
using uvec2 = linalg::aliases::uint2;
using uvec3 = linalg::aliases::uint3;
using uvec4 = linalg::aliases::uint4;

namespace tim
{
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

    template<typename TO, typename FROM>
    TO union_cast(FROM _from)
    {
        union
        {
            FROM a;
            TO b;
        } res;
        res.a = _from;
        return res.b;
    }

    template<typename T>
    constexpr T alignUp(T x, T align)
    {
        return x + (x % align == 0 ? 0 : align - x % align);
    }

    template<typename T, typename A>
    constexpr T * alignUpPtr(T * _ptr, A align)
    {
        std::uintptr_t ptr = reinterpret_cast<std::uintptr_t>(_ptr);
        ptr = ptr + (ptr % align == 0 ? 0 : align - ptr % align);
        return reinterpret_cast<T*>(ptr);
    }

    template<typename T>
    constexpr T absolute_difference(T a, T b)
    {
        return a < b ? b - a : a - b;
    }
}