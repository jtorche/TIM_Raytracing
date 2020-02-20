#pragma once
#include "rtRenderer/public/IRenderer.h"
#include "ShaderCompiler/ShaderCompiler.h"

class PostprocessPass
{
public:
    PostprocessPass(tim::IRenderer* _renderer, tim::IRenderContext * _context);
    ~PostprocessPass();

    void setFrameBufferSize(tim::uvec2 _res);

    void linearToSrgb(tim::ImageHandle _inColorBuffer, tim::ImageHandle _outBackbuffer);

private:


private:
    tim::uvec2 m_frameSize;
    tim::IRenderer * m_renderer = nullptr;
    tim::IRenderContext * m_context = nullptr;


};