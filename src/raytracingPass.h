#pragma once
#include "rtRenderer/public/IRenderer.h"
#include "ShaderCompiler/ShaderCompiler.h"

class SimpleCamera;
class BVHBuilder;

class RayTracingPass
{
public:
    RayTracingPass(tim::IRenderer* _renderer, tim::IRenderContext * _context);
    ~RayTracingPass();
    
    void beginRender();

    void rebuildBvh(tim::u32 _maxDepth, tim::u32 _maxObjPerNode);
    void setFrameBufferSize(tim::uvec2 _res);
    void draw(tim::ImageHandle _output, const SimpleCamera& _camera);

private:
    tim::u32 getRayStorageBufferSize() const;
    tim::BufferHandle getRayStorageBuffer();
    tim::ImageHandle getColorBuffer();

    void drawBounce(tim::u32 _depth, tim::BufferView _passData, tim::BufferHandle _inputRayBuffer, tim::ImageHandle _outputImage);

private:
    tim::u32 m_rayBounceRecursionDepth;
    tim::uvec2 m_frameSize;
    tim::IRenderer* m_renderer = nullptr;
    tim::IRenderContext * m_context = nullptr;
    std::unique_ptr<BVHBuilder> m_bvh;
    tim::BufferHandle m_bvhBuffer;
    tim::uvec2 m_bvhPrimitiveOffsetRange;
    tim::uvec2 m_bvhMaterialOffsetRange;
    tim::uvec2 m_bvhLightOffsetRange;
    tim::uvec2 m_bvhNodeOffsetRange;
    tim::uvec2 m_bvhLeafDataOffsetRange;

    std::vector<tim::BufferHandle> m_allocatedRayStorageBuffer;
    std::vector<tim::BufferHandle> m_availableRayStorageBuffer;
    std::vector<tim::ImageHandle> m_allocatedColorBuffer;
    std::vector<tim::ImageHandle> m_availableColorBuffer;
};