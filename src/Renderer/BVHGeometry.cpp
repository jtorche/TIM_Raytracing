#include "BVHGeometry.h"
#include "timCore/Common.h"

namespace tim
{

	BVHGeometry::BVHGeometry(IRenderer* _renderer, u32 _numVertexInBuffer) : m_renderer{ _renderer }, m_maxVertexCount{ _numVertexInBuffer }
	{
		u32 totalBufferSize = (sizeof(vec3) + sizeof(vec3) + sizeof(vec2)) * m_maxVertexCount;
		m_gpuBuffer = m_renderer->CreateBuffer(totalBufferSize, MemoryType::Default, BufferUsage::Storage | BufferUsage::Transfer);
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

    u32 BVHGeometry::addTriangleList(u32 _numVertex, const vec3 * _positions, const vec3 * _normals, const vec2 * _texCoords)
    {
        u32 vertexOffset = (u32)m_cpuBufferPosition.size() / 3;

        for (u32 i = 0; i < _numVertex; ++i)
        {
            m_cpuBufferPosition.push_back(_positions[i]);
            m_cpuBufferNormal.push_back(_normals[i]);
            if (_texCoords)
                m_cpuBufferUv.push_back(_texCoords[i]);
            else
                m_cpuBufferUv.push_back({ 0,0 });
        }

        return vertexOffset;
    }

    vec3 BVHGeometry::getVertexPosition(u32 _vertexOffet, u32 _index) const
    {
        return m_cpuBufferPosition[_vertexOffet + _index];
    }

	void BVHGeometry::flush(IRenderer* _renderer)
	{
		_renderer->UploadBuffer(m_gpuBuffer, 0, &m_cpuBufferPosition[0], (u32)m_cpuBufferPosition.size() * sizeof(vec3));
		_renderer->UploadBuffer(m_gpuBuffer, m_maxVertexCount * sizeof(vec3), &m_cpuBufferNormal[0], (u32)m_cpuBufferNormal.size() * sizeof(vec3));
		_renderer->UploadBuffer(m_gpuBuffer, m_maxVertexCount * (sizeof(vec3) + sizeof(vec3)), &m_cpuBufferUv[0], (u32)m_cpuBufferUv.size() * sizeof(vec2));
	}

    void BVHGeometry::generateGeometryBufferBindings(BufferBinding& _positions, BufferBinding& _normals, BufferBinding& _texcoords)
    {
        _positions = { { m_gpuBuffer, 0, m_maxVertexCount * sizeof(vec3) }, { 1, 0 } };
        _normals = { { m_gpuBuffer, m_maxVertexCount * sizeof(vec3), m_maxVertexCount * sizeof(vec3) }, { 1, 1 } };
        _texcoords = { { m_gpuBuffer, m_maxVertexCount * (sizeof(vec3) + sizeof(vec3)), m_maxVertexCount * sizeof(vec2) }, { 1, 2 } };
    }
}