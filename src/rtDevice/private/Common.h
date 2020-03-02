#pragma once
#include <iostream>
#include "timCore/Common.h"
#include <vulkan/vulkan.h>

#ifdef _DEBUG
#define TIM_VK_ASSERT(result) \
    do { if((result) != VK_SUCCESS) { handleAssert(__LINE__, __FILE__, toString(result)); } }while(0)

#define TIM_VK_VERIFY(result) \
    do { if((result) != VK_SUCCESS) { handleAssert(__LINE__, __FILE__, toString(result)); } }while(0)

#else
#define TIM_VK_ASSERT(result)
#define TIM_VK_VERIFY(result) result
#define TIM_ASSERT(cond)
#endif

#define TIM_FRAME_LATENCY 2

namespace tim
{
    const char* toString(VkResult _res);

    template<typename T>
    constexpr std::underlying_type_t<T> to_integral(T _x)
    {
        return static_cast<std::underlying_type_t<T>>(_x);
    }
}
