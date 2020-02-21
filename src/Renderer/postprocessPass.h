#pragma once
#include "rtDevice/public/IRenderer.h"
#include "ShaderCompiler/ShaderCompiler.h"

namespace tim
{
    class PostprocessPass
    {
    public:
        PostprocessPass(IRenderer* _renderer, IRenderContext* _context);
        ~PostprocessPass();

        void setFrameBufferSize(uvec2 _res);

        void linearToSrgb(ImageHandle _inColorBuffer, ImageHandle _outBackbuffer);

    private:


    private:
        uvec2 m_frameSize;
        IRenderer* m_renderer = nullptr;
        IRenderContext* m_context = nullptr;


    };
}