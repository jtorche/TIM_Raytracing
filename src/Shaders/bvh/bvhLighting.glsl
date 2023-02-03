#ifndef H_BVHLIGHTING_FXH_
#define H_BVHLIGHTING_FXH_

#include "bvhGetter.glsl"
#include "core/lighting.glsl"
#include "lightprob/lightprobHelpers.glsl"
#include "lightprob/fetchSH_inline.glsl"

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

vec3 computeLPFLighting(uint _rootId, in Ray _ray, vec3 _diffuse, in LightProbFieldHeader _lpfHeader, in ClosestHit _hit)
{
#ifdef NO_LPF
	return vec3(0, 0, 0);
#else
	vec3 pos = _ray.from + _ray.dir * _hit.t;
	vec3 uvw = getLightProbCoordUVW(pos, _lpfHeader.resolution, _lpfHeader.aabb);

	// SH9Color sh = fetchSH9(ivec3(uvw.x+0.5, uvw.y+0.5, uvw.z+0.5));
	SH9Color sh = sampleSH9(uvw,  _lpfHeader.resolution);

	vec3 L = evalSH(sh, getHitNormal(_hit));
	uint matId = getMaterialId(_hit);

	return computeIndirectLighting(g_BvhMaterialData[matId], _diffuse, L);
#endif
}

vec3 computeLighting(uint _rootId, in Ray _ray, in SunDirColor _sun, in LightProbFieldHeader _lpfHeader, in ClosestHit _hit)
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
	//for(uint i=0 ; i<numLights ; ++i)
	//{
	//	uint lightIndex = g_BvhLeafData[1 + leafDataOffset + numTriangles + numBlas + numObjects + i];
	//	lit += evalLighting(_rootId, lightIndex, matId, diffuse, _ray, _hit);
	//}

#if SHOW_LPF_DEBUG
	if(_hit.nid != 0xFFFFffff) // special value for light prob debug
#endif
	{
		for(uint i=0 ; i<g_Constants.numLights ; ++i)
			lit += evalLighting(_rootId, i, matId, diffuse, _ray, _hit);

		lit += computeSunLighting(_rootId, _sun.sunDir, _sun.sunColor, matId, diffuse, _ray, _hit);
	}

#ifdef USE_LPF
	lit += computeLPFLighting(_rootId, _ray, diffuse, _lpfHeader, _hit);
#endif

	return lit;
}

#endif