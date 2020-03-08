#ifndef H_BVHBINDINGS_FXH_
#define H_BVHBINDINGS_FXH_

#include "struct_cpp.glsl"
#include "primitive_cpp.glsl"
#include "collision.glsl"

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

layout(set = 0, binding = g_dataTextures_bind) uniform sampler2D g_dataTextures[TEXTURE_ARRAY_SIZE];
// layout(set = 0, binding = g_dataTextures_bind) uniform sampler2D g_dataTexture;

#if USE_SHARED_MEM
shared vec3 g_normalHit[LOCAL_SIZE * LOCAL_SIZE];
shared vec2 g_uvHit[LOCAL_SIZE * LOCAL_SIZE];
#endif

vec3 getHitNormal(in ClosestHit _hit)
{
#if USE_SHARED_MEM
    return g_normalHit[gl_LocalInvocationIndex];
#else
    return _hit.normal;
#endif
}

vec2 getHitUv(in ClosestHit _hit)
{
#if USE_SHARED_MEM
    return g_uvHit[gl_LocalInvocationIndex];
#else
    return _hit.uv;
#endif
}

void storeHitNormal(inout ClosestHit _hit, vec3 _normal)
{
#if USE_SHARED_MEM
    g_normalHit[gl_LocalInvocationIndex] = _normal;
#else
    _hit.normal = _normal;
#endif
}

void storeHitUv(inout ClosestHit _hit, vec2 _uv)
{
#if USE_SHARED_MEM
    g_uvHit[gl_LocalInvocationIndex] = _uv;
#else
    _hit.uv = _uv;
#endif
}
#endif