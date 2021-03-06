#include "raytracingPass.h"
#include "SimpleCamera.h"
#include "BVHBuilder.h"
#include "shaderMacros.h"
#include "Scene.h"
#include "Shaders/struct_cpp.glsl"
#include "TextureManager.h"

#include <iostream>

namespace tim
{
    RayTracingPass::RayTracingPass(IRenderer* _renderer, IRenderContext* _context, ResourceAllocator& _allocator, TextureManager& _texManager) 
        : m_frameSize{ 800,600 }, m_renderer{ _renderer }, m_context{ _context }, m_resourceAllocator{ _allocator }, m_textureManager{ _texManager }
    {
        m_rayBounceRecursionDepth = 2;
        m_geometryBuffer = std::make_unique<BVHGeometry>(_renderer, 1024 * 1024);
        m_bvh = std::make_unique<BVHBuilder>(*m_geometryBuffer);

        Scene scene(*m_geometryBuffer.get(), m_textureManager);
        scene.build(m_bvh.get());

        m_geometryBuffer->flush(m_renderer);

        {
            auto start = std::chrono::system_clock::now();
            m_bvh->build(17, 12, Box{ vec3{ -100,-100,-100 }, vec3{ 100,100,100 } });
            m_bvh->dumpStats();
            auto end = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsed_seconds = end - start;
            std::cout << "Build BVH time: " << elapsed_seconds.count() << "s\n";
        }

        u32 size = m_bvh->getBvhGpuSize();
        std::cout << "Uploading " << (size >> 10) << " Ko of BVH data\n";
        m_bvhBuffer = _renderer->CreateBuffer(size, MemoryType::Default, BufferUsage::Storage | BufferUsage::Transfer);
        std::unique_ptr<ubyte[]> buffer = std::unique_ptr<ubyte[]>(new ubyte[size]);
        m_bvh->fillGpuBuffer(buffer.get(), m_bvhTriangleOffsetRange, m_bvhPrimitiveOffsetRange, m_bvhMaterialOffsetRange, m_bvhLightOffsetRange, m_bvhNodeOffsetRange, m_bvhLeafDataOffsetRange);
        _renderer->UploadBuffer(m_bvhBuffer, buffer.get(), size);

        system("pause");
    }

    RayTracingPass::~RayTracingPass()
    {
        m_renderer->DestroyBuffer(m_bvhBuffer);
    }

    void RayTracingPass::rebuildBvh(u32 _maxDepth, u32 _maxObjPerNode)
    {
        if (m_bvhBuffer.ptr != 0)
        {
            m_renderer->WaitForIdle();
            m_renderer->DestroyBuffer(m_bvhBuffer);
        }

        u32 size = m_bvh->getBvhGpuSize();
        m_bvhBuffer = m_renderer->CreateBuffer(size, MemoryType::Default, BufferUsage::Storage | BufferUsage::Transfer);
        std::unique_ptr<ubyte[]> buffer = std::unique_ptr<ubyte[]>(new ubyte[size]);
        m_bvh->fillGpuBuffer(buffer.get(), m_bvhTriangleOffsetRange, m_bvhPrimitiveOffsetRange, m_bvhMaterialOffsetRange, m_bvhLightOffsetRange, m_bvhNodeOffsetRange, m_bvhLeafDataOffsetRange);
        m_renderer->UploadBuffer(m_bvhBuffer, buffer.get(), size);
    }

    void RayTracingPass::setFrameBufferSize(uvec2 _res)
    {
        m_frameSize = _res;
    }

    u32 RayTracingPass::getRayStorageBufferSize() const
    {
        return m_frameSize.x * m_frameSize.y * sizeof(IndirectLightRay);
    }

    void RayTracingPass::draw(ImageHandle _outputBuffer, const SimpleCamera& _camera)
    {
        const float fov = 70.f * 3.14f / 180;

        PassData passData;
        passData.frameSize = { m_frameSize.x, m_frameSize.y };
        passData.invFrameSize = { 1.f / m_frameSize.x, 1.f / m_frameSize.y };
        passData.cameraPos = { _camera.getPos(), 0 };
        passData.cameraDir = { _camera.getDir(), 0 };

        mat4 projMat = linalg::perspective_matrix<float>(fov, float(m_frameSize.x) / m_frameSize.y, 0.1f, TMAX, linalg::neg_z, linalg::zero_to_one);
        mat4 viewProj = linalg::mul(projMat, _camera.getViewMat());
        passData.invProjView = linalg::inverse(viewProj);

        passData.frustumCorner00 = linalg::mul(passData.invProjView, vec4(-1, -1, 1, 1));
        passData.frustumCorner10 = linalg::mul(passData.invProjView, vec4(1, -1, 1, 1));
        passData.frustumCorner01 = linalg::mul(passData.invProjView, vec4(-1, 1, 1, 1));

        passData.frustumCorner00 /= passData.frustumCorner00.w;
        passData.frustumCorner10 /= passData.frustumCorner10.w;
        passData.frustumCorner01 /= passData.frustumCorner01.w;

        auto w = passData.frustumCorner10 - passData.frustumCorner00;
        auto h = passData.frustumCorner01 - passData.frustumCorner00;

        BufferView passDataBuffer;
        void* passDataPtr = m_renderer->GetDynamicBuffer(sizeof(PassData), passDataBuffer);
        memcpy(passDataPtr, &passData, sizeof(PassData));

        BufferHandle reflexionRayBuffer = m_resourceAllocator.allocBuffer(getRayStorageBufferSize(), BufferUsage::Storage | BufferUsage::Transfer, MemoryType::Default);
        BufferHandle refractionRayBuffer = m_resourceAllocator.allocBuffer(getRayStorageBufferSize(), BufferUsage::Storage | BufferUsage::Transfer, MemoryType::Default);
        m_context->ClearBuffer(reflexionRayBuffer, 0);
        m_context->ClearBuffer(refractionRayBuffer, 0);

        DrawArguments arg = {};
        std::vector<ImageBinding> imgBinds = {
            { _outputBuffer, ImageViewType::Storage, 0, g_outputImage_bind }
        };
        m_textureManager.fillImageBindings(imgBinds);

        BufferBinding geometryPos, geometryNormals, geometryTexcoords;
        m_geometryBuffer->generateGeometryBufferBindings(geometryPos, geometryNormals, geometryTexcoords);

        BufferBinding bufBinds[] = {
            { passDataBuffer, { 0, g_PassData_bind } },
            { { m_bvhBuffer, m_bvhPrimitiveOffsetRange.x, m_bvhPrimitiveOffsetRange.y }, { 0, g_BvhPrimitives_bind } },
            { { m_bvhBuffer, m_bvhTriangleOffsetRange.x, m_bvhTriangleOffsetRange.y }, { 0, g_BvhTriangles_bind } },
            { { m_bvhBuffer, m_bvhMaterialOffsetRange.x, m_bvhMaterialOffsetRange.y }, { 0, g_BvhMaterials_bind } },
            { { m_bvhBuffer, m_bvhLightOffsetRange.x, m_bvhLightOffsetRange.y }, { 0, g_BvhLights_bind } },
            { { m_bvhBuffer, m_bvhNodeOffsetRange.x, m_bvhNodeOffsetRange.y }, { 0, g_BvhNodes_bind } },
            { { m_bvhBuffer, m_bvhLeafDataOffsetRange.x, m_bvhLeafDataOffsetRange.y }, { 0, g_BvhLeafData_bind } },
            { { reflexionRayBuffer, 0, getRayStorageBufferSize() }, { 0, g_OutReflexionRayBuffer_bind } },
            { { refractionRayBuffer, 0, getRayStorageBufferSize() }, { 0, g_OutRefractionRayBuffer_bind } },
            geometryPos, geometryNormals, geometryTexcoords
        };

        arg.m_imageBindings = &imgBinds[0];
        arg.m_numImageBindings = (u32)imgBinds.size();
        arg.m_bufferBindings = bufBinds;
        arg.m_numBufferBindings = _countof(bufBinds);

        PushConstants constants = { m_bvh->getTrianglesCount(), m_bvh->getPrimitivesCount(), m_bvh->getLightsCount(), m_bvh->getNodesCount() };
        arg.m_constants = &constants;
        arg.m_constantSize = sizeof(constants);
        ShaderFlags flags;
        flags.set(C_FIRST_RECURSION_STEP);
        arg.m_key = { TIM_HASH32(cameraPass.comp), flags };

        const u32 localSize = LOCAL_SIZE;
        m_context->Dispatch(arg, alignUp<u32>(m_frameSize.x, localSize) / localSize, alignUp<u32>(m_frameSize.y, localSize) / localSize);

        if (m_rayBounceRecursionDepth > 1)
        {
            drawBounce(1, passDataBuffer, reflexionRayBuffer, _outputBuffer);
            drawBounce(1, passDataBuffer, refractionRayBuffer, _outputBuffer);
        }

        m_resourceAllocator.releaseBuffer(reflexionRayBuffer);
        m_resourceAllocator.releaseBuffer(refractionRayBuffer);
    }

    void RayTracingPass::drawBounce(u32 _depth, BufferView _passData, BufferHandle _inputRayBuffer, ImageHandle _curImage)
    {
        ImageHandle mainColorBuffer = _curImage;

        DrawArguments arg = {};
        std::vector<ImageBinding> imgBinds = {
            { mainColorBuffer, ImageViewType::Storage, 0, g_outputImage_bind },
            { mainColorBuffer, ImageViewType::Storage, 0, g_inputImage_bind }
        };
        m_textureManager.fillImageBindings(imgBinds);

        BufferBinding geometryPos, geometryNormals, geometryTexcoords;
        m_geometryBuffer->generateGeometryBufferBindings(geometryPos, geometryNormals, geometryTexcoords);

        std::vector<BufferBinding> bufBinds = {
            { _passData, { 0, g_PassData_bind } },
            { { m_bvhBuffer, m_bvhPrimitiveOffsetRange.x, m_bvhPrimitiveOffsetRange.y }, { 0, g_BvhPrimitives_bind } },
            { { m_bvhBuffer, m_bvhMaterialOffsetRange.x, m_bvhMaterialOffsetRange.y }, { 0, g_BvhMaterials_bind } },
            { { m_bvhBuffer, m_bvhLightOffsetRange.x, m_bvhLightOffsetRange.y }, { 0, g_BvhLights_bind } },
            { { m_bvhBuffer, m_bvhNodeOffsetRange.x, m_bvhNodeOffsetRange.y }, { 0, g_BvhNodes_bind } },
            { { m_bvhBuffer, m_bvhLeafDataOffsetRange.x, m_bvhLeafDataOffsetRange.y }, { 0, g_BvhLeafData_bind } },
            { { _inputRayBuffer, 0, getRayStorageBufferSize() }, { 0, g_InRayBuffer_bind } },
            geometryPos, geometryNormals, geometryTexcoords
        };

        BufferHandle outRayBuffer;
        ShaderFlags flags;
        if (_depth + 1 < m_rayBounceRecursionDepth)
        {
            flags.set(C_CONTINUE_RECURSION);
            outRayBuffer = m_resourceAllocator.allocBuffer(getRayStorageBufferSize(), BufferUsage::Storage | BufferUsage::Transfer, MemoryType::Default);
            m_context->ClearBuffer(outRayBuffer, 0);
            bufBinds.push_back({ { outRayBuffer, 0, getRayStorageBufferSize() }, { 0, g_OutRayBuffer_bind } });
        }

        arg.m_imageBindings = &imgBinds[0];
        arg.m_numImageBindings = (u32)imgBinds.size();
        arg.m_bufferBindings = &bufBinds[0];
        arg.m_numBufferBindings = (u32)bufBinds.size();

        PushConstants constants = { m_bvh->getTrianglesCount(), m_bvh->getPrimitivesCount(), m_bvh->getLightsCount(), m_bvh->getNodesCount() };
        arg.m_constants = &constants;
        arg.m_constantSize = sizeof(constants);
        arg.m_key = { TIM_HASH32(rayBouncePass.comp), flags };

        const u32 localSize = LOCAL_SIZE;
        m_context->Dispatch(arg, alignUp<u32>(m_frameSize.x, localSize) / localSize, alignUp<u32>(m_frameSize.y, localSize) / localSize);

        if (_depth + 1 < m_rayBounceRecursionDepth)
        {
            drawBounce(_depth + 1, _passData, outRayBuffer, _curImage);
            m_resourceAllocator.releaseBuffer(outRayBuffer);
        }
    }
}