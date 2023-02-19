#ifndef H_BVHBINDINGS_FXH_
#define H_BVHBINDINGS_FXH_

#include "bvhBindings_cpp.glsl"
#include "core/primitive_cpp.glsl"
#include "core/collision.glsl"

layout(std430, set = 0, binding = g_BvhPrimitives_bind) buffer BvhPrimitives
{
	PackedPrimitive g_BvhPrimitiveData[];
};

layout(std430, set = 0, binding = g_BvhTriangles_bind) buffer BvhTriangles
{
	Triangle g_BvhTriangleData[];
};

layout(std430, set = 0, binding = g_BvhMaterials_bind) buffer BvhMaterials
{	
	Material g_BvhMaterialData[];
};

layout(std430, set = 0, binding = g_BvhLights_bind) buffer BvhLights
{
	PackedLight g_BvhLightData[];
};

layout(std430, set = 0, binding = g_BvhNodes_bind) buffer BvhNodes
{
	PackedBVHNode g_BvhNodeData[];
};

layout(std430, set = 0, binding = g_BlasHeaders_bind) buffer BlasHeaders
{
    BlasHeader g_blasHeader[];
};

layout(std430, set = 0, binding = g_BvhLeafData_bind) buffer BvhLeafData
{
	uint g_BvhLeafData[];
};

// Geometry data
layout(std430, set = 1, binding = 0) buffer GeometryData_Position
{
	float g_positionData[];
};
layout(std430, set = 1, binding = 1) buffer GeometryData_Normal
{
	float g_normalData[];
};
layout(std430, set = 1, binding = 2) buffer GeometryData_TexCoord
{
	float g_texCoordData[];
};

void loadTriangleVertices(out vec3 p0, out vec3 p1, out vec3 p2, uint index0, uint index1, uint index2)
{
	p0 = vec3(g_positionData[index0 * 3], g_positionData[index0 * 3 + 1], g_positionData[index0 * 3 + 2]);
	p1 = vec3(g_positionData[index1 * 3], g_positionData[index1 * 3 + 1], g_positionData[index1 * 3 + 2]);
	p2 = vec3(g_positionData[index2 * 3], g_positionData[index2 * 3 + 1], g_positionData[index2 * 3 + 2]);
}

void loadNormalVertices(out vec3 n0, out vec3 n1, out vec3 n2, uint index0, uint index1, uint index2)
{
	n0 = vec3(g_normalData[index0 * 3], g_normalData[index0 * 3 + 1], g_normalData[index0 * 3 + 2]);
	n1 = vec3(g_normalData[index1 * 3], g_normalData[index1 * 3 + 1], g_normalData[index1 * 3 + 2]);
	n2 = vec3(g_normalData[index2 * 3], g_normalData[index2 * 3 + 1], g_normalData[index2 * 3 + 2]);
}

void loadUvVertices(out vec2 uv0, out vec2 uv1, out vec2 uv2, uint index0, uint index1, uint index2)
{
	uv0 = vec2(g_texCoordData[index0 * 2], g_texCoordData[index0 * 2 + 1]);
	uv1 = vec2(g_texCoordData[index1 * 2], g_texCoordData[index1 * 2 + 1]);
	uv2 = vec2(g_texCoordData[index2 * 2], g_texCoordData[index2 * 2 + 1]);
}

layout(set = 0, binding = g_dataTextures_bind) uniform sampler2D g_dataTextures[TEXTURE_ARRAY_SIZE];

// CLosest hit shared memory interface
#if USE_SHARED_MEM
shared uvec4 g_hitData[NUM_THREADS_PER_GROUP];
#endif

uvec4 ClosestHit_getHitData(in ClosestHit _hit)
{
#if USE_SHARED_MEM
	return uvec4(g_hitData[gl_LocalInvocationIndex].xyz, floatBitsToUint(_hit.t));
#else
	return uvec4(_hit.triangle.vertexOffset, _hit.triangle.index01, _hit.triangle.index2_matId, floatBitsToUint(_hit.t));
#endif
}

Triangle ClosestHit_getTriangle(in ClosestHit _hit)
{
#if USE_SHARED_MEM
	Triangle tri;
	tri.vertexOffset = g_hitData[gl_LocalInvocationIndex].x;
	tri.index01 = g_hitData[gl_LocalInvocationIndex].y;
	tri.index2_matId = g_hitData[gl_LocalInvocationIndex].z;
	return tri;
#else
	return _hit.triangle;
#endif
}

void ClosestHit_storeTriangle(inout ClosestHit _hit, in Triangle _triangle)
{
#if USE_SHARED_MEM
	g_hitData[gl_LocalInvocationIndex].x = _triangle.vertexOffset;
	g_hitData[gl_LocalInvocationIndex].y = _triangle.index01;
	g_hitData[gl_LocalInvocationIndex].z = _triangle.index2_matId;
#else
	_hit.triangle = _triangle;
#endif
}

uint ClosestHit_getMaterialId(in ClosestHit _hit)
{
#if USE_SHARED_MEM
	return (g_hitData[gl_LocalInvocationIndex].z & 0xFFFF0000) >> 16;
#else
	return (_hit.triangle.index2_matId & 0xFFFF0000) >> 16;
#endif
}

void ClosestHit_storeNid(inout ClosestHit _hit, in uint _nid)
{
#if USE_SHARED_MEM
	g_hitData[gl_LocalInvocationIndex].w = _nid;
#else
	_hit.nid = _nid;
#endif
}

uint ClosestHit_getNid(in ClosestHit _hit)
{
#if USE_SHARED_MEM
	return g_hitData[gl_LocalInvocationIndex].w;
#else
	return _hit.nid;
#endif
}

#endif