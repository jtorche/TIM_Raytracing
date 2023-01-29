#include "timCore/Common.h"
#include "BVHData.h"
#include "Shaders/struct_cpp.glsl"

namespace tim
{
    BVHData::BVHData(IRenderer* _renderer, const BVHGeometry& _geometry) : m_renderer{ _renderer }, m_geometry{ _geometry }
    { 
    }

	BVHData::~BVHData()
	{
		m_renderer->DestroyBuffer(m_bvhBuffer);
	}

    void BVHData::fillBvhBindings(std::vector<BufferBinding>& _bindings) const
    {
        _bindings.push_back({ { m_bvhBuffer, m_bvhPrimitiveOffsetRange.x, m_bvhPrimitiveOffsetRange.y }, { 0, g_BvhPrimitives_bind } });
        _bindings.push_back({ { m_bvhBuffer, m_bvhTriangleOffsetRange.x, m_bvhTriangleOffsetRange.y }, { 0, g_BvhTriangles_bind } });
        _bindings.push_back({ { m_bvhBuffer, m_bvhMaterialOffsetRange.x, m_bvhMaterialOffsetRange.y }, { 0, g_BvhMaterials_bind } });
        _bindings.push_back({ { m_bvhBuffer, m_bvhLightOffsetRange.x, m_bvhLightOffsetRange.y }, { 0, g_BvhLights_bind } });
        _bindings.push_back({ { m_bvhBuffer, m_bvhNodeOffsetRange.x, m_bvhNodeOffsetRange.y }, { 0, g_BvhNodes_bind } });
        _bindings.push_back({ { m_bvhBuffer, m_bvhBlasHeaderDataOffsetRange.x, m_bvhBlasHeaderDataOffsetRange.y }, { 0, g_BlasHeaders_bind } });
        _bindings.push_back({ { m_bvhBuffer, m_bvhLeafDataOffsetRange.x, m_bvhLeafDataOffsetRange.y }, { 0, g_BvhLeafData_bind } });
    }

    void BVHData::build(BVHBuilder& _builder, const BVHBuildParameters& _bvhParams, const BVHBuildParameters& _tlasParams, bool _useTlasBlas)
    {
        {
            auto start = std::chrono::system_clock::now();
            _builder.buildBlas(_bvhParams);
        
            const bool multithread = true;
            _builder.setParameters(_useTlasBlas ? _tlasParams : _bvhParams);
            _builder.build(multithread);
            _builder.dumpStats();
            auto end = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsed_seconds = end - start;
            std::cout << "Build BVH time: " << elapsed_seconds.count() << "s\n";
        }
        
        u32 size = _builder.getBvhGpuSize();
        std::cout << "Uploading " << (size >> 10) << " Ko of BVH data\n";
        m_bvhBuffer = m_renderer->CreateBuffer(size, MemoryType::Default, BufferUsage::Storage | BufferUsage::Transfer);
        std::unique_ptr<ubyte[]> buffer = std::unique_ptr<ubyte[]>(new ubyte[size]);
        _builder.fillGpuBuffer(buffer.get(), m_bvhTriangleOffsetRange, m_bvhPrimitiveOffsetRange, m_bvhMaterialOffsetRange, m_bvhLightOffsetRange, m_bvhNodeOffsetRange, m_bvhLeafDataOffsetRange, m_bvhBlasHeaderDataOffsetRange);
        m_renderer->UploadBuffer(m_bvhBuffer, buffer.get(), size);
    }
}