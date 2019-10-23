#pragma once
#include <iostream>

#ifdef _DEBUG
#define TIM_ASSERT(cond) \
    do { if(!(cond)) { tim::handleAssert(__LINE__, __FILE__, ""); } }while(0) 

#else
#define TIM_ASSERT(cond)
#endif

#define TIM_FRAME_LATENCY 2

namespace tim
{
    void handleAssert(int _line, const char* _file, const char* _msg);
}
