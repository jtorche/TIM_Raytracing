#include "raytracingPass.h"
#include "TextureManager.h"
#include "SimpleCamera.h"
#include "BVHData.h"
#include "shaderMacros.h"
#include "Scene.h"
#include "Shaders/struct_cpp.glsl"
#include "Shaders/bvh/bvhBindings_cpp.glsl"

#include <iostream>

namespace tim
{
    RayTracingPass::RayTracingPass(IRenderer* _renderer, IRenderContext* _context, ResourceAllocator& _allocator, TextureManager& _texManager)
        : m_frameSize{ 800,600 }, m_renderer{ _renderer }, m_context{ _context }, m_resourceAllocator{ _allocator }, m_textureManager{ _texManager }
    {
        m_rayBounceRecursionDepth = 2;
    }

    RayTracingPass::~RayTracingPass()
    {
    }

    void RayTracingPass::setFrameBufferSize(uvec2 _res)
    {
        m_frameSize = _res;
    }

    void RayTracingPass::setBounceRecursionDepth(u32 _depth)
    {
        m_rayBounceRecursionDepth = _depth;
    }

    u32 RayTracingPass::getRayStorageBufferSize() const
    {
        return m_frameSize.x * m_frameSize.y * sizeof(IndirectLightRay);
    }

    RayTracingPass::PassResource RayTracingPass::fillPassResources(const SimpleCamera& _camera, const Scene& _scene)
    {
        PassResource resources;

        const float fov = 70.f * 3.14f / 180;

        PassData passData;
        passData.frameSize = { m_frameSize.x, m_frameSize.y };
        passData.invFrameSize = { 1.f / m_frameSize.x, 1.f / m_frameSize.y };
        passData.cameraPos = { _camera.getPos(), 0 };
        passData.cameraDir = { _camera.getDir(), 0 };
        passData.sunDir = { normalize(_scene.getSunData().sunDir), 0 };
        passData.sunColor = { _scene.getSunData().sunColor, 0 };

        mat4 projMat = linalg::perspective_matrix<float>(fov, float(m_frameSize.x) / m_frameSize.y, 0.1f, TMAX, linalg::neg_z, linalg::zero_to_one);
        mat4 viewProj = linalg::mul(projMat, _camera.getViewMat());
        passData.invProjView = linalg::inverse(viewProj);

        passData.frustumCorner00 = linalg::mul(passData.invProjView, vec4(-1, -1, 0.5f, 1));
        passData.frustumCorner10 = linalg::mul(passData.invProjView, vec4(1, -1, 0.5f, 1));
        passData.frustumCorner01 = linalg::mul(passData.invProjView, vec4(-1, 1, 0.5f, 1));

        passData.frustumCorner00 /= passData.frustumCorner00.w;
        passData.frustumCorner10 /= passData.frustumCorner10.w;
        passData.frustumCorner01 /= passData.frustumCorner01.w;

        passData.sceneMinExtent = { _scene.getAABB().minExtent, 0 };
        passData.sceneMaxExtent = { _scene.getAABB().maxExtent, 0 };
        passData.lpfResolution = { _scene.getLPF().m_fieldSize, 0 };

        void* passDataPtr = m_renderer->GetDynamicBuffer(sizeof(PassData), resources.m_passData);
        memcpy(passDataPtr, &passData, sizeof(PassData));

        const u32 tracingResultBufferSize = m_frameSize.x * m_frameSize.y * sizeof(uvec4) * 2;
        BufferHandle tracingResultBuffer = m_resourceAllocator.allocBuffer(tracingResultBufferSize, BufferUsage::Storage | BufferUsage::Transfer, MemoryType::Default);
        resources.m_tracingResult = { tracingResultBuffer, 0, tracingResultBufferSize };
    
        resources.m_reflexionRayBuffer = m_resourceAllocator.allocBuffer(getRayStorageBufferSize(), BufferUsage::Storage | BufferUsage::Transfer, MemoryType::Default);
        resources.m_refractionRayBuffer = m_resourceAllocator.allocBuffer(getRayStorageBufferSize(), BufferUsage::Storage | BufferUsage::Transfer, MemoryType::Default);
    
        return resources;
    }

    void RayTracingPass::freePassResources(const PassResource& _resources)
    {
        m_resourceAllocator.releaseBuffer(_resources.m_tracingResult.m_buffer);
        m_resourceAllocator.releaseBuffer(_resources.m_reflexionRayBuffer);
        m_resourceAllocator.releaseBuffer(_resources.m_refractionRayBuffer);
    }

    void RayTracingPass::tracePass(ImageHandle _outputBuffer, const Scene& _scene, const PassResource& _resources)
    {
        PushConstants cst;
        std::vector<BufferBinding> bufBinds;
        std::vector<ImageBinding> imgBinds = { 
            { _outputBuffer, ImageViewType::Storage, 0, g_outputImage_bind } 
        };

        DrawArguments arg = fillBindings(_scene, _resources, cst, bufBinds, imgBinds);

        ShaderFlags flags;
        flags.set(C_TRACING_STEP);
        flags.set(C_USE_LPF_MASK);

        if (_scene.useTlas())
            flags.set(C_USE_TRAVERSE_TLAS);

        const u32 localSize = LOCAL_SIZE;

        arg.m_key = { TIM_HASH32(cameraPass.comp), flags };
        m_context->Dispatch(arg, alignUp<u32>(m_frameSize.x, localSize) / localSize, alignUp<u32>(m_frameSize.y, localSize) / localSize);
    }

    void RayTracingPass::lightingPass(ImageHandle _outputBuffer, const Scene& _scene, const PassResource& _resources)
    {
        m_context->ClearBuffer(_resources.m_reflexionRayBuffer, 0);
        m_context->ClearBuffer(_resources.m_refractionRayBuffer, 0);

        PushConstants cst;
        std::vector<BufferBinding> bufBinds;
        std::vector<ImageBinding> imgBinds = {
            { _outputBuffer, ImageViewType::Storage, 0, g_outputImage_bind }
        };

        DrawArguments arg = fillBindings(_scene, _resources, cst, bufBinds, imgBinds);

        ShaderFlags flags;
        flags.set(C_FIRST_RECURSION_STEP);
        flags.set(C_USE_LPF);
        flags.set(C_USE_LPF_MASK);

        if (_scene.useTlas())
            flags.set(C_USE_TRAVERSE_TLAS);

        const u32 localSize = LOCAL_SIZE;

        arg.m_key = { TIM_HASH32(cameraPass.comp), flags };
        m_context->Dispatch(arg, alignUp<u32>(m_frameSize.x, localSize) / localSize, alignUp<u32>(m_frameSize.y, localSize) / localSize);
    }

    DrawArguments RayTracingPass::fillBindings(const Scene& _scene, const PassResource& _resources, PushConstants& _cst, std::vector<BufferBinding>& _bufBinds, std::vector<ImageBinding>& _imgBinds)
    {
        DrawArguments arg = {};

        _scene.getLPF().fillBindings(_imgBinds, g_lpfTextures_bind);
        m_textureManager.fillImageBindings(_imgBinds);

        _bufBinds = {
            { _resources.m_passData, { 0, g_PassData_bind } },
            { { _resources.m_reflexionRayBuffer, 0, getRayStorageBufferSize() }, { 0, g_OutReflexionRayBuffer_bind } },
            { { _resources.m_refractionRayBuffer, 0, getRayStorageBufferSize() }, { 0, g_OutRefractionRayBuffer_bind } },
            { _resources.m_tracingResult, { 0, g_tracingResult_bind } }
        };

        _scene.fillGeometryBufferBindings(_bufBinds);
        _scene.getBVH().fillBvhBindings(_bufBinds);

        _cst = { _scene.getTrianglesCount(), _scene.getBlasInstancesCount(), _scene.getPrimitivesCount(), _scene.getLightsCount(), _scene.getNodesCount() };
        arg.m_constants = &_cst;
        arg.m_constantSize = sizeof(PushConstants);

        arg.m_imageBindings = _imgBinds.data();
        arg.m_numImageBindings = (u32)_imgBinds.size();
        arg.m_bufferBindings = _bufBinds.data();
        arg.m_numBufferBindings = (u32)_bufBinds.size();

        return arg;
    }

    void RayTracingPass::drawBounce(const Scene& _scene, u32 _depth, BufferView _passData, BufferHandle _inputRayBuffer, ImageHandle _curImage)
    {
        ImageHandle mainColorBuffer = _curImage;

        DrawArguments arg = {};
        std::vector<ImageBinding> imgBinds = {
            { mainColorBuffer, ImageViewType::Storage, 0, g_outputImage_bind },
            { mainColorBuffer, ImageViewType::Storage, 0, g_inputImage_bind }
        };
        m_textureManager.fillImageBindings(imgBinds);
        _scene.getLPF().fillBindings(imgBinds, g_lpfTextures_bind);

        std::vector<BufferBinding> bufBinds = {
            { _passData, { 0, g_PassData_bind } },
            { { _inputRayBuffer, 0, getRayStorageBufferSize() }, { 0, g_InRayBuffer_bind } }
        };

        BufferHandle outRayBuffer;
        ShaderFlags flags;
        flags.set(C_USE_LPF);

        if (_depth + 1 < m_rayBounceRecursionDepth)
        {
            flags.set(C_CONTINUE_RECURSION);
            outRayBuffer = m_resourceAllocator.allocBuffer(getRayStorageBufferSize(), BufferUsage::Storage | BufferUsage::Transfer, MemoryType::Default);
            m_context->ClearBuffer(outRayBuffer, 0);
            bufBinds.push_back({ { outRayBuffer, 0, getRayStorageBufferSize() }, { 0, g_OutRayBuffer_bind } });
        }
        if (_scene.useTlas())
            flags.set(C_USE_TRAVERSE_TLAS);

        _scene.fillGeometryBufferBindings(bufBinds);
        _scene.getBVH().fillBvhBindings(bufBinds);

        arg.m_imageBindings = &imgBinds[0];
        arg.m_numImageBindings = (u32)imgBinds.size();
        arg.m_bufferBindings = &bufBinds[0];
        arg.m_numBufferBindings = (u32)bufBinds.size();

        PushConstants constants = { _scene.getTrianglesCount(), _scene.getBlasInstancesCount(), _scene.getPrimitivesCount(), _scene.getLightsCount(), _scene.getNodesCount() };
        arg.m_constants = &constants;
        arg.m_constantSize = sizeof(constants);
        arg.m_key = { TIM_HASH32(rayBouncePass.comp), flags };

        const u32 localSize = LOCAL_SIZE;
        m_context->Dispatch(arg, alignUp<u32>(m_frameSize.x, localSize) / localSize, alignUp<u32>(m_frameSize.y, localSize) / localSize);

        if (_depth + 1 < m_rayBounceRecursionDepth)
        {
            drawBounce(_scene, _depth + 1, _passData, outRayBuffer, _curImage);
            m_resourceAllocator.releaseBuffer(outRayBuffer);
        }
    }
}
