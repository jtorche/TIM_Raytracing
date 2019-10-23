#include "rtRenderer/public/IRenderer.h"
#include "ShaderCompiler/ShaderCompiler.h"

class SimpleCamera;
class BVHBuilder;

class RayTracingPass
{
public:
    RayTracingPass(tim::uvec2 _frameSize, tim::IRenderer* _renderer, tim::IRenderContext * _context);
    ~RayTracingPass();

    void draw(tim::ImageHandle _output, const SimpleCamera& _camera);

private:
    tim::uvec2 m_frameSize;
    tim::IRenderer* m_renderer = nullptr;
    tim::IRenderContext * m_context = nullptr;
    std::unique_ptr<BVHBuilder> m_bvh;
    tim::BufferHandle m_bvhBuffer;
    tim::uvec2 m_bvhPrimitiveOffsetRange;
    tim::uvec2 m_bvhNodeOffsetRange;
    tim::uvec2 m_bvhLeafDataOffsetRange;
};