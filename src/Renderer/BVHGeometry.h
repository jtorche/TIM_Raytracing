#pragma once
#include "rtDevice/public/IRenderer.h"

namespace tim
{
	class BVHGeometry
	{
	public:
		BVHGeometry(IRenderer* _renderer, u32 _numVertexInBuffer);
		~BVHGeometry();

		struct TriangleData
		{
			u32 vertexOffset;
			uint16_t index[3];
		};
		TriangleData addTriangle(vec3 _p0, vec3 _p1, vec3 _p2);
        
        vec3 getVertexPosition(u32 _vertexOffet, u32 _index) const;

		void flush(IRenderer* _renderer);
        void generateGeometryBufferBindings(BufferBinding& _positions, BufferBinding& _normals, BufferBinding& _texcoords);

	private:
		IRenderer* m_renderer;
		u32 m_maxVertexCount;
		BufferHandle m_gpuBuffer;
		std::vector<vec3> m_cpuBufferPosition;
		std::vector<vec3> m_cpuBufferNormal;
		std::vector<vec2> m_cpuBufferUv;
	};
}