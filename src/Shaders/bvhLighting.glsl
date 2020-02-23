#ifndef H_BVHLIGHTING_FXH_
#define H_BVHLIGHTING_FXH_

#include "bvhGetter.glsl"
#include "lighting.glsl"

vec3 evalLighting(uint _rootId, uint lightIndex, uint _matId, in Ray _ray, in ClosestHit _hit)
{
#if USE_SHARED_MEM
	vec3 normal = g_normalHit[gl_LocalInvocationIndex];
#else
	vec3 normal = _hit.normal;	
#endif

	switch(g_BvhLightData[lightIndex].iparam)
	{
		case Light_Sphere:
		return evalSphereLight(_rootId, loadSphereLight(lightIndex), g_BvhMaterialData[_matId], _ray.from + _ray.dir * _hit.t, normal, vec3(0,0,0));
		case Light_Area:
		return evalAreaLight(_rootId, loadAreaLight(lightIndex), g_BvhMaterialData[_matId], _ray.from + _ray.dir * _hit.t, normal, vec3(0,0,0));
	}

	return vec3(0,0,0);
}

vec3 computeDirectLighting(uint rootId, in Ray _ray, in ClosestHit _hit)
{
	uint nid = _hit.nid_mid & 0xFFFF;
	uint matId = getMaterialId(_hit);

	uint leafDataOffset = g_BvhNodeData[nid].nid.z;
	uvec3 unpacked = unpackObjectCount(g_BvhNodeData[nid].nid.w);
	uint numTriangles = unpacked.x;
	uint numObjects = unpacked.y;
	uint numLights = unpacked.z;

	vec3 totalLight = vec3(0,0,0);
	for(uint i=0 ; i<numLights ; ++i)
	{
		uint lightIndex = g_BvhLeafData[leafDataOffset + numObjects + numTriangles + i];
		totalLight += evalLighting(rootId, lightIndex, matId, _ray, _hit);
	}

	return totalLight;
}

#endif