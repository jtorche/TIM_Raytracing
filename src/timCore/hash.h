#pragma once
#include "type.h"
#include <stdint.h>
#include <functional>

namespace tim
{
    namespace detail
    {
        constexpr uint32_t val_32_const = 0x811c9dc5;
        constexpr uint32_t prime_32_const = 0x1000193;
        constexpr uint64_t val_64_const = 0xcbf29ce484222325;
        constexpr uint64_t prime_64_const = 0x100000001b3;

        inline constexpr uint32_t hash_32_fnv1a_const(const char* const str, const uint32_t value = val_32_const) noexcept
        {
            return (str[0] == '\0') ? value : hash_32_fnv1a_const(&str[1], (value ^ uint32_t(str[0])) * prime_32_const);
        }

        inline constexpr uint64_t hash_64_fnv1a_const(const char* const str, const uint64_t value = val_64_const) noexcept
        {
            return (str[0] == '\0') ? value : hash_64_fnv1a_const(&str[1], (value ^ uint64_t(str[0])) * prime_64_const);
        }

        inline constexpr uint32_t hash_32_fnv1a_const(const wchar_t* const str, const uint32_t value = val_32_const) noexcept
        {
            return (str[0] == '\0') ? value : hash_32_fnv1a_const(&str[1], (value ^ uint32_t(str[0])) * prime_32_const);
        }

        inline constexpr uint64_t hash_64_fnv1a_const(const wchar_t* const str, const uint64_t value = val_64_const) noexcept
        {
            return (str[0] == '\0') ? value : hash_64_fnv1a_const(&str[1], (value ^ uint64_t(str[0])) * prime_64_const);
        }
    }

    #define TIM_HASH32(str) ::tim::detail::hash_32_fnv1a_const(#str)
    #define TIM_HASH32_STR(str) ::tim::detail::hash_32_fnv1a_const(str)

    // Helpers to combine hashes
    inline void hash_combine(std::size_t& seed) { }

    template <typename T, typename... Rest>
    inline void hash_combine(std::size_t& seed, const T& v, Rest... rest) 
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        hash_combine(seed, rest...);
    }
}