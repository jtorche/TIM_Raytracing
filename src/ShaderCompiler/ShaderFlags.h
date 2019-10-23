#pragma once
#include "timCore/hash.h"
#include <functional>

namespace tim
{
    using FxNameHash = u32;

    struct ShaderFlags
    {
        static const u32 MaxFlags = 64;

        void set(u32 _flag) { m_flags = m_flags | (u64(1) << _flag); }
        bool test(u32 _flag) { return (m_flags & (u64(1) << _flag)) > 0; }

        bool operator==(ShaderFlags _other) const { return m_flags == _other.m_flags; }

        u64 m_flags = 0;
    };

    struct ShaderKey
    {
        bool operator==(const ShaderKey& _other) const { return fx == _other.fx && flags == _other.flags; }

        FxNameHash fx = 0;
        ShaderFlags flags;
    };
}

namespace std
{
    template<> struct hash<tim::ShaderKey>
    {
        size_t operator()(const tim::ShaderKey& key) const
        {
            size_t h = 0;
            tim::hash_combine(h, key.fx, key.flags.m_flags);
            return h;
        }
    };
}