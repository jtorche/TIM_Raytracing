#include "postprocessPass.h"
#include "shaderMacros.h"

#include "Shaders/struct_cpp.glsl"

namespace tim
{
    PostprocessPass::PostprocessPass(IRenderer* _renderer, IRenderContext* _context) : m_renderer{ _renderer }, m_context{ _context }
    {

    }

    PostprocessPass::~PostprocessPass()
    {

    }

    void PostprocessPass::setFrameBufferSize(uvec2 _res)
    {
        m_frameSize = _res;
    }

    void PostprocessPass::linearToSrgb(ImageHandle _inColorBuffer, ImageHandle _outBackbuffer)
    {
        DrawArguments arg = {};
        ImageBinding imgBinds[] = {
            { _inColorBuffer, ImageViewType::Storage, { 0, 0 } },
            { _outBackbuffer, ImageViewType::Storage, { 0, 1 } }
        };
        arg.m_imageBindings = imgBinds;
        arg.m_numImageBindings = _countof(imgBinds);

        ivec4 cst = { (int)m_frameSize.x, (int)m_frameSize.y, 0, 0 };
        arg.m_constants = &cst;
        arg.m_constantSize = sizeof(cst);
        arg.m_key = { TIM_HASH32(linearToSrgb.comp), {} };
        m_context->Dispatch(arg, alignUp<u32>(m_frameSize.x, LOCAL_SIZE) / LOCAL_SIZE, alignUp<u32>(m_frameSize.y, LOCAL_SIZE) / LOCAL_SIZE);
    }

    void PostprocessPass::copyImage(ImageHandle _src, ImageHandle _dst, uvec2 _dstSize)
    {
        DrawArguments arg = {};
        ImageBinding imgBinds[] = 
        {
            { _src, ImageViewType::Sampled, { 0, 0 }, SamplerType::Clamp_Nearest_MipNearest },
            { _dst, ImageViewType::Storage, { 0, 1 } }
        };
        arg.m_imageBindings = imgBinds;
        arg.m_numImageBindings = _countof(imgBinds);

        ivec4 cst = { (int)_dstSize.x, (int)_dstSize.y, 0, 0 };
        arg.m_constants = &cst;
        arg.m_constantSize = sizeof(cst);
        arg.m_key = { TIM_HASH32(copyImage.comp), {} };
        m_context->Dispatch(arg, alignUp<u32>(_dstSize.x, LOCAL_SIZE) / LOCAL_SIZE, alignUp<u32>(_dstSize.y, LOCAL_SIZE) / LOCAL_SIZE);
    }

    void PostprocessPass::tonemapPass(ImageHandle _src, ImageHandle _dst)
    {
        DrawArguments arg = {};
        ImageBinding imgBinds[] = {
            { _src, ImageViewType::Storage, { 0, 0 } },
            { _dst, ImageViewType::Storage, { 0, 1 } }
        };
        arg.m_imageBindings = imgBinds;
        arg.m_numImageBindings = _countof(imgBinds);

        ivec4 cst = { (int)m_frameSize.x, (int)m_frameSize.y, 0, 0 };
        arg.m_constants = &cst;
        arg.m_constantSize = sizeof(cst);
        arg.m_key = { TIM_HASH32(tonemap.comp), {} };
        m_context->Dispatch(arg, alignUp<u32>(m_frameSize.x, LOCAL_SIZE) / LOCAL_SIZE, alignUp<u32>(m_frameSize.y, LOCAL_SIZE) / LOCAL_SIZE);
    }
}