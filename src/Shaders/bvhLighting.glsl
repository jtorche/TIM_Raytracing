#ifndef H_BVHLIGHTING_FXH_
#define H_BVHLIGHTING_FXH_

#include "bvhGetter.glsl"
#include "lighting.glsl"

vec3 evalLighting(uint _rootId, uint lightIndex, uint _matId, in Ray _ray, in ClosestHit _hit)
{
	vec3 normal = getHitNormal(_hit);
	vec3 texColor = getHitColor(_hit);

	switch(g_BvhLightData[lightIndex].iparam)
	{
		case Light_Sphere:
		return evalSphereLight(_rootId, loadSphereLight(lightIndex), g_BvhMaterialData[_matId], texColor, _ray.from + _ray.dir * _hit.t, normal, _ray.from);
		case Light_Area:
		return evalAreaLight(_rootId, loadAreaLight(lightIndex), g_BvhMaterialData[_matId], texColor, _ray.from + _ray.dir * _hit.t, normal, _ray.from);
	}

	return vec3(0,0,0);
}

vec3 computeDirectLighting(uint rootId, in Ray _ray, in ClosestHit _hit)
{
	uint matId = getMaterialId(_hit);

	uint leafDataOffset = g_BvhNodeData[_hit.nid].nid.w;
	uvec3 unpacked = unpackObjectCount(g_BvhLeafData[leafDataOffset]);
	uint numTriangles = unpacked.x;
	uint numObjects = unpacked.y;
	uint numLights = unpacked.z;

	vec3 totalLight = vec3(0,0,0);
	for(uint i=0 ; i<numLights ; ++i)
	{
		uint lightIndex = g_BvhLeafData[1 + leafDataOffset + numTriangles + numObjects + i];
		totalLight += evalLighting(rootId, lightIndex, matId, _ray, _hit);
	}

	return totalLight;
}

vec3 computeSunLighting(uint _rootId, vec3 _sunDir, vec3 _sunColor, uint _matId, in Ray _ray, in ClosestHit _hit)
{
	vec3 normal = getHitNormal(_hit);
	vec3 texColor = getHitColor(_hit);

	return evalSunLight(_rootId, -_sunDir, _sunColor, g_BvhMaterialData[_matId], texColor,
		                _ray.from + _ray.dir * _hit.t, normal, _ray.from);
}

#endif