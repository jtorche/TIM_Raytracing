#ifndef H_BVHBINDINGS_FXH_
#define H_BVHBINDINGS_FXH_

#include "struct_cpp.glsl"
#include "primitive_cpp.glsl"
#include "collision.glsl"

layout(std430, set = 0, binding = g_BvhPrimitives_bind) buffer BvhPrimitives
{
	PackedPrimitive g_BvhPrimitiveData[];
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

#if USE_SHARED_MEM
shared vec3 g_normalHit[LOCAL_SIZE * LOCAL_SIZE];
#endif

vec3 getHitNormal(in ClosestHit _hit)
{
#if USE_SHARED_MEM
    return g_normalHit[gl_LocalInvocationIndex];
#else
    return _hit.normal;
#endif
}

#endif