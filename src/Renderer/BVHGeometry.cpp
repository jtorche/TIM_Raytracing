#include "BVHGeometry.h"
#include "timCore/Common.h"

namespace tim
{

	BVHGeometry::BVHGeometry(IRenderer* _renderer, u32 _numVertexInBuffer) : m_renderer{ _renderer }, m_maxVertexCount{ _numVertexInBuffer }
	{
		u32 totalBufferSize = (sizeof(vec3) + sizeof(vec3) + sizeof(vec2)) * m_maxVertexCount;
		m_gpuBuffer = m_renderer->CreateBuffer(totalBufferSize, MemoryType::Default, BufferUsage::Storage);
	}

	BVHGeometry::~BVHGeometry()
	{
		m_renderer->DestroyBuffer(m_gpuBuffer);
	}

	BVHGeometry::TriangleData BVHGeometry::addTriangle(vec3 _p0, vec3 _p1, vec3 _p2)
	{
		TIM_ASSERT(m_cpuBufferPosition.size() + 3 < m_maxVertexCount);

		TriangleData triangle;
		triangle.vertexOffset = (u32)m_cpuBufferPosition.size() / 3;
		triangle.index[0] = 0;
		triangle.index[1] = 1;
		triangle.index[2] = 2;

		m_cpuBufferPosition.push_back(_p0);
		m_cpuBufferPosition.push_back(_p1);
		m_cpuBufferPosition.push_back(_p2);

		vec3 normal = linalg::cross((_p1 - _p0), (_p2 - _p0));
		normal = linalg::normalize(normal);

		m_cpuBufferNormal.push_back(normal);
		m_cpuBufferNormal.push_back(normal);
		m_cpuBufferNormal.push_back(normal);

		m_cpuBufferUv.push_back({ 0,0 });
		m_cpuBufferUv.push_back({ 0,1 });
		m_cpuBufferUv.push_back({ 1,0 });

		return triangle;
	}

	void BVHGeometry::flush(IRenderer* _renderer)
	{
		_renderer->UploadBuffer(m_gpuBuffer, 0, &m_cpuBufferPosition[0], (u32)m_cpuBufferPosition.size() * sizeof(vec3));
		_renderer->UploadBuffer(m_gpuBuffer, m_maxVertexCount * sizeof(vec3), &m_cpuBufferNormal[0], (u32)m_cpuBufferNormal.size() * sizeof(vec3));
		_renderer->UploadBuffer(m_gpuBuffer, m_maxVertexCount * (sizeof(vec3) + sizeof(vec3)), &m_cpuBufferUv[0], (u32)m_cpuBufferUv.size() * sizeof(vec2));
	}

}