#include "postprocessPass.h"
#include "shaderMacros.h"

using namespace tim;
#include "Shaders/struct_cpp.glsl"

PostprocessPass::PostprocessPass(tim::IRenderer* _renderer, tim::IRenderContext* _context) : m_renderer{ _renderer }, m_context{ _context }
{

}

PostprocessPass::~PostprocessPass()
{

}

void PostprocessPass::setFrameBufferSize(tim::uvec2 _res)
{
    m_frameSize = _res;
}

void PostprocessPass::linearToSrgb(tim::ImageHandle _inColorBuffer, tim::ImageHandle _outBackbuffer)
{
    DrawArguments arg = {};
    ImageBinding imgBinds[] = {
        { _inColorBuffer, ImageViewType::Storage, 0, 0 },
        { _outBackbuffer, ImageViewType::Storage, 0, 1 }
    };
    arg.m_imageBindings = imgBinds;
    arg.m_numImageBindings = _countof(imgBinds);

    ivec4 cst = { (int)m_frameSize.x, (int)m_frameSize.y, 0, 0 };
    arg.m_constants = &cst;
    arg.m_constantSize = sizeof(cst);
    arg.m_key = { TIM_HASH32(linearToSrgb.comp), {} };
    m_context->Dispatch(arg, alignUp<u32>(m_frameSize.x, LOCAL_SIZE) / LOCAL_SIZE, alignUp<u32>(m_frameSize.y, LOCAL_SIZE) / LOCAL_SIZE);
}