#ifndef H_BVHBINDINGS_FXH_
#define H_BVHBINDINGS_FXH_

#include "struct_cpp.fxh"

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


#endif