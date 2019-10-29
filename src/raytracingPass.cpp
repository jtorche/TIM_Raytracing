#include "raytracingPass.h"
#include "SimpleCamera.h"
#include "BVHBuilder.h"

using namespace tim;
#include "Shaders/struct_cpp.fxh"

RayTracingPass::RayTracingPass(IRenderer* _renderer, IRenderContext* _context) : m_frameSize{ 800,600 },  m_renderer { _renderer }, m_context{ _context }
{
    m_bvh = std::make_unique<BVHBuilder>(1,0);

    m_bvh->addSphere({ {  3, 3, 0.5f }, 1 });
    m_bvh->addSphere({ { -3, 3, 0.5f }, 1 });
    m_bvh->addSphere({ {  3,-3, 0.5f }, 1 });
    m_bvh->addSphere({ { -3,-3, 0.5f }, 1 });
    m_bvh->addSphere({ { -3, 0, 0.25f }, 0.5 });
    m_bvh->addSphere({ {  3, 0, 0.25f }, 0.5 });
    m_bvh->addSphere({ { 0,0, 1 }, 1 });
    m_bvh->addBox(Box{ { -5, -5, -0.1f }, { 5, 5, 0.1f } });
    m_bvh->addPointLight({ { 0, 0, 5 }, 5, { 1, 1, 1 } });

    m_bvh->build(Box{ vec3{ -5,-5,-5 }, vec3{5,5,5} });

    u32 size = m_bvh->getBvhGpuSize();
    m_bvhBuffer = _renderer->CreateBuffer(size, MemoryType::Default, BufferUsage::Storage | BufferUsage::Transfer);
    std::unique_ptr<ubyte[]> buffer = std::unique_ptr<ubyte[]>(new ubyte[size]);
    m_bvh->fillGpuBuffer(buffer.get(), m_bvhPrimitiveOffsetRange, m_bvhLightOffsetRange, m_bvhNodeOffsetRange, m_bvhLeafDataOffsetRange);
    _renderer->UploadBuffer(m_bvhBuffer, buffer.get(), size);
}

RayTracingPass::~RayTracingPass()
{
    m_renderer->DestroyBuffer(m_bvhBuffer);
}

void RayTracingPass::setFrameBufferSize(tim::uvec2 _res)
{
    m_frameSize = _res;
}

void RayTracingPass::draw(tim::ImageHandle _output, const SimpleCamera& _camera)
{
    const float fov = 70.f * 3.14f / 180;

    PassData passData;
    passData.frameSize = { m_frameSize.x, m_frameSize.y };
    passData.invFrameSize = { 1.f / m_frameSize.x, 1.f / m_frameSize.y };
    passData.cameraPos = { _camera.getPos(), 0 };

    mat4 projMat = linalg::perspective_matrix<float>(fov, float(m_frameSize.x) / m_frameSize.y, 1, 10, linalg::neg_z, linalg::zero_to_one);
    mat4 viewProj = linalg::mul(projMat, _camera.getViewMat());
    passData.invProjView = linalg::inverse(viewProj);

    passData.frustumCorner00 = linalg::mul(passData.invProjView, vec4(-1, -1, 1, 1));
    passData.frustumCorner10 = linalg::mul(passData.invProjView, vec4( 1, -1, 1, 1));
    passData.frustumCorner01 = linalg::mul(passData.invProjView, vec4(-1,  1, 1, 1));

    passData.frustumCorner00 /= passData.frustumCorner00.w;
    passData.frustumCorner10 /= passData.frustumCorner10.w;
    passData.frustumCorner01 /= passData.frustumCorner01.w;

    auto w = passData.frustumCorner10 - passData.frustumCorner00;
    auto h = passData.frustumCorner01 - passData.frustumCorner00;

    BufferView passDataBuffer;
    void * passDataPtr = m_renderer->GetDynamicBuffer(sizeof(PassData), passDataBuffer);
    memcpy(passDataPtr, &passData, sizeof(PassData));

    DrawArguments arg = {};
    ImageBinding imgBinds[] = {
        { _output, ImageViewType::Storage, 0, g_outputImage_bind }
    };
    BufferBinding bufBinds[] = {
        { passDataBuffer, { 0, g_PassData_bind } },
        { { m_bvhBuffer, m_bvhPrimitiveOffsetRange.x, m_bvhPrimitiveOffsetRange.y }, { 0, g_BvhPrimitives_bind } },
        { { m_bvhBuffer, m_bvhLightOffsetRange.x, m_bvhLightOffsetRange.y }, { 0, g_BvhLights_bind } },
        { { m_bvhBuffer, m_bvhNodeOffsetRange.x, m_bvhNodeOffsetRange.y }, { 0, g_BvhNodes_bind } },
        { { m_bvhBuffer, m_bvhLeafDataOffsetRange.x, m_bvhLeafDataOffsetRange.y }, { 0, g_BvhLeafData_bind } },
    };

    arg.m_imageBindings = imgBinds;
    arg.m_numImageBindings = _countof(imgBinds);
    arg.m_bufferBindings = bufBinds;
    arg.m_numBufferBindings = _countof(bufBinds);

    u32 constants[] = { m_bvh->getPrimitivesCount(), m_bvh->getNodesCount() };
    arg.m_constants = constants;
    arg.m_constantSize = sizeof(u32) * _countof(constants);
    arg.m_key = { TIM_HASH32(cameraPass.comp), {} };

    m_context->Dispatch(arg, alignUp<u32>(m_frameSize.x, 16) / 16, alignUp<u32>(m_frameSize.y, 16) / 16);
}