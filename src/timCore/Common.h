#pragma once
#include <iostream>

#ifdef _DEBUG
#define TIM_ASSERT(cond) \
    do { if(!(cond)) { handleAssert(__LINE__, __FILE__, ""); } }while(0) 

#else
#define TIM_ASSERT(cond)
#endif

#define TIM_FRAME_LATENCY 2

namespace tim
{
    void handleAssert(int _line, const char* _file, const char* _msg);

    template<typename T>
    bool isPowerOf2(T _val) { return (_val & (_val - 1)) == 0; }
}
