#ifndef H_BVHLIGHTING_FXH_
#define H_BVHLIGHTING_FXH_

#include "bvhGetter.glsl"
#include "lighting.glsl"

vec3 evalLighting(uint _rootId, uint _lightIndex, uint _matId, vec3 _diffuse, in Ray _ray, in ClosestHit _hit)
{
	vec3 normal = getHitNormal(_hit);

	switch(g_BvhLightData[_lightIndex].iparam)
	{
		case Light_Sphere:
		return evalSphereLight(_rootId, loadSphereLight(_lightIndex), g_BvhMaterialData[_matId], _diffuse, _ray.from + _ray.dir * _hit.t, normal, _ray.from);
		case Light_Area:
		return evalAreaLight(_rootId, loadAreaLight(_lightIndex), g_BvhMaterialData[_matId], _diffuse, _ray.from + _ray.dir * _hit.t, normal, _ray.from);
	}

	return vec3(0,0,0);
}

vec3 computeSunLighting(uint _rootId, vec3 _sunDir, vec3 _sunColor, uint _matId, vec3 _diffuse, in Ray _ray, in ClosestHit _hit)
{
	vec3 normal = getHitNormal(_hit);

	return evalSunLight(_rootId, _sunDir, _sunColor, g_BvhMaterialData[_matId], _diffuse,
		                _ray.from + _ray.dir * _hit.t, normal, _ray.from);
}

vec3 computeDirectLighting(uint _rootId, in Ray _ray, in PassData _passData, in ClosestHit _hit)
{
	vec3 diffuse = vec3(1,1,1);

	uint matId = getMaterialId(_hit);
#if DYNAMIC_TEXTURE_INDEXING
	uint diffuseMap = g_BvhMaterialData[matId].type_ids.y & 0xFFFF;
	if (diffuseMap < 0xFFFF)
		diffuse = texture(g_dataTextures[nonuniformEXT(diffuseMap)], getHitUv(_hit)).xyz;
#endif


	uint leafDataOffset = g_BvhNodeData[_hit.nid].nid.w;
	uvec4 unpacked = unpackObjectCount(g_BvhLeafData[leafDataOffset]);
	uint numTriangles = unpacked.x;
	uint numBlas = unpacked.y;
	uint numObjects = unpacked.y;
	uint numLights = unpacked.w;

	vec3 lit = vec3(0,0,0);
	for(uint i=0 ; i<numLights ; ++i)
	{
		uint lightIndex = g_BvhLeafData[1 + leafDataOffset + numTriangles + numBlas + numObjects + i];
		lit += evalLighting(_rootId, lightIndex, matId, diffuse, _ray, _hit);
	}

	//for(uint i=0 ; i<g_Constants.numLights ; ++i)
	//	lit += evalLighting(_rootId, i, matId, diffuse, _ray, _hit);

	lit += computeSunLighting(_rootId, _passData.sunDir.xyz, _passData.sunColor.xyz, matId, diffuse, _ray, _hit);

	return lit;
}

#endif