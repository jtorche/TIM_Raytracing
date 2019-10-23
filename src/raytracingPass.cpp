#include "raytracingPass.h"
#include "SimpleCamera.h"
#include "BVHBuilder.h"

using namespace tim;
#include "Shaders/struct_cpp.fxh"

RayTracingPass::RayTracingPass(uvec2 _frameSize, IRenderer* _renderer, IRenderContext* _context) : m_frameSize{ _frameSize },  m_renderer { _renderer }, m_context{ _context }
{
    m_bvh = std::make_unique<BVHBuilder>();

    m_bvh->addSphere({ {  3, 3, 0.5f }, 1 });
    m_bvh->addSphere({ { -3, 3, 0.5f }, 1 });
    m_bvh->addSphere({ {  3,-3, 0.5f }, 1 });
    m_bvh->addSphere({ { -3,-3, 0.5f }, 1 });
    m_bvh->addSphere({ { -3, 0, 0.25f }, 0.5 });
    m_bvh->addSphere({ {  3, 0, 0.25f }, 0.5 });
    m_bvh->addSphere({ { 0,0, 1 }, 2 });
    m_bvh->addSphere({ { 0,0, 1.25f }, 1.5 });
    m_bvh->addSphere({ { 0,0, 1.35f }, 1 });
    m_bvh->addSphere({ { 0,0, 1.5f }, 0.7f });

    m_bvh->build(Box{ vec3{ -5,-5,-5 }, vec3{5,5,5} });

    u32 size = m_bvh->getBvhGpuSize();
    m_bvhBuffer = _renderer->CreateBuffer(size, MemoryType::Default, BufferUsage::Storage | BufferUsage::Transfer);
    std::unique_ptr<ubyte[]> buffer = std::unique_ptr<ubyte[]>(new ubyte[size]);
    m_bvh->fillGpuBuffer(buffer.get(), m_bvhPrimitiveOffsetRange, m_bvhNodeOffsetRange, m_bvhLeafDataOffsetRange);
    _renderer->UploadBuffer(m_bvhBuffer, buffer.get(), size);
}

RayTracingPass::~RayTracingPass()
{
    m_renderer->DestroyBuffer(m_bvhBuffer);
}

float RayBoxIntersect(vec3 rpos, vec3 rdir, vec3 vmin, vec3 vmax)
{
    float t1 = (vmin.x - rpos.x) / rdir.x;
    float t2 = (vmax.x - rpos.x) / rdir.x;
    float t3 = (vmin.y - rpos.y) / rdir.y;
    float t4 = (vmax.y - rpos.y) / rdir.y;
    float t5 = (vmin.z - rpos.z) / rdir.z;
    float t6 = (vmax.z - rpos.z) / rdir.z;

    float aMin = t1 < t2 ? t1 : t2;
    float bMin = t3 < t4 ? t3 : t4;
    float cMin = t5 < t6 ? t5 : t6;

    float aMax = t1 > t2 ? t1 : t2;
    float bMax = t3 > t4 ? t3 : t4;
    float cMax = t5 > t6 ? t5 : t6;

    float fMax = aMin > bMin ? aMin : bMin;
    float fMin = aMax < bMax ? aMax : bMax;

    float t7 = fMax > cMin ? fMax : cMin;
    float t8 = fMin < cMax ? fMin : cMax;

    float t9 = (t8 < 0 || t7 > t8) ? -1 : t7;

    return t9;
}

void RayTracingPass::draw(tim::ImageHandle _output, const SimpleCamera& _camera)
{
    vec3 rpos = { 3,1,3 }; vec3 rdir = { -1,0,0 };
    vec3 minEx = {-1,-1,-1};
    vec3 maxEx = { 1,1,1 };
    float r = RayBoxIntersect(rpos, rdir, minEx, maxEx);

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
        { { m_bvhBuffer, m_bvhNodeOffsetRange.x, m_bvhNodeOffsetRange.y }, { 0, g_BvhNodes_bind } },
        { { m_bvhBuffer, m_bvhLeafDataOffsetRange.x, m_bvhLeafDataOffsetRange.y }, { 0, g_BvhLeafData_bind } },
    };

    arg.m_imageBindings = imgBinds;
    arg.m_numImageBindings = 1;
    arg.m_bufferBindings = bufBinds;
    arg.m_numBufferBindings = 1;
    arg.m_key = { TIM_HASH32(cameraPass.comp), {} };

    m_context->Dispatch(arg, alignUp<u32>(m_frameSize.x, 16) / 16, alignUp<u32>(m_frameSize.y, 16) / 16);
}