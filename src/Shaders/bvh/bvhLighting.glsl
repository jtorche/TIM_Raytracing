#ifndef H_BVHLIGHTING_FXH_
#define H_BVHLIGHTING_FXH_

#include "bvhGetter.glsl"
#include "core/lighting.glsl"
#include "lightprob/lightprobHelpers.glsl"
#include "lightprob/fetchSH_inline.glsl"

vec3 evalLighting(uint _rootId, uint _lightIndex, uint _matId, vec3 _texColor, vec3 _pos, vec3 _eye, vec3 _normal)
{
	switch(g_BvhLightData[_lightIndex].iparam)
	{
		case Light_Sphere:
		return evalSphereLight(_rootId, loadSphereLight(_lightIndex), g_BvhMaterialData[_matId], _texColor, _pos, _normal, _eye);
		//case Light_Area:
		//return evalAreaLight(_rootId, loadAreaLight(_lightIndex), g_BvhMaterialData[_matId], _texColor, _pos, _normal, _eye);
	}

	return vec3(0,0,0);
}

vec3 computeLPFLighting(uint _rootId, in LightProbFieldHeader _lpfHeader, uint _matId, uint _lpfMask, vec3 _texColor, vec3 _pos, vec3 _normal)
{
#ifdef NO_LPF
	return vec3(0, 0, 0);
#else
	vec3 uvw = getLightProbCoordUVW(_pos, _lpfHeader.resolution, _lpfHeader.aabb, _lpfHeader.step);

	#if USE_LPF_MASK
	SH9Color sh = sampleSH9(uvw, _lpfHeader.resolution, _lpfMask);
	// SH9Color sh = sampleSH9_test(uvw, _lpfHeader.resolution, _lpfMask);
	#else
	SH9Color sh = sampleSH9(uvw, _lpfHeader.resolution);
	// SH9Color sh = sampleSH9_test(uvw, _lpfHeader.resolution);
	#endif

	vec3 L = evalSH(sh, _normal);
	return computeIndirectLighting(g_BvhMaterialData[_matId], _texColor, L);
#endif
}

vec3 computeLighting(uint _rootId, in SunDirColor _sun, in LightProbFieldHeader _lpfHeader, uvec2 _lightingMask, uint _matId, vec3 _pos, vec3 _eye, vec3 _normal, vec2 _uv)
{
	vec3 texColor = vec3(1,1,1);

#if DYNAMIC_TEXTURE_INDEXING
	uint diffuseMap = g_BvhMaterialData[_matId].type_ids.y & 0xFFFF;
	if (diffuseMap < 0xFFFF)
		texColor *= texture(g_dataTextures[nonuniformEXT(diffuseMap)], _uv).xyz;
#endif

	//uint leafDataOffset = g_BvhNodeData[_hit.nid].nid.w;
	//uvec4 unpacked = unpackObjectCount(g_BvhLeafData[leafDataOffset]);
	//uint numTriangles = unpacked.x;
	//uint numBlas = unpacked.y;
	//uint numObjects = unpacked.y;
	//uint numLights = unpacked.w;

	vec3 lit = vec3(0,0,0);
	
	//for(uint i=0 ; i<numLights ; ++i)
	//{
	//	uint lightIndex = g_BvhLeafData[1 + leafDataOffset + numTriangles + numBlas + numObjects + i];
	//	lit += evalLighting(_rootId, lightIndex, matId, diffuse, _ray, _hit);
	//}

	for(uint i=0 ; i<g_Constants.numLights ; ++i)
	{
		if((_lightingMask.x & (1u << (i + 1))) == 0)
			lit += evalLighting(_rootId, i, _matId, texColor, _pos, _eye, _normal);
	}

	if((_lightingMask.x & 1) == 0)
		lit += evalSunLight(_rootId, _sun.sunDir, _sun.sunColor, g_BvhMaterialData[_matId], texColor, _pos, _eye, _normal);

#ifdef USE_LPF
	lit += computeLPFLighting(_rootId, _lpfHeader, _matId, _lightingMask.y, texColor, _pos, _normal);
#endif

	return lit;
}

Ray createShadowRay(vec3 _pos, vec3 _dir)
{
	Ray shadowRay; 
	shadowRay.from = _pos; 
	shadowRay.dir = _dir;
	
#if STORE_RAY_INVDIR   
	shadowRay.invdir = vec3(1,1,1) / _dir;
#endif
	
	return shadowRay;
}

bool traverseForShadow(in Ray _ray, float _rayLength)
{
	uint rootId = g_Constants.numNodes == 1 ? NID_LEAF_BIT : 0;
#ifdef USE_TRAVERSE_TLAS
	return traverseTlasFast(_ray, rootId, _rayLength);
#else
	return traverseBvhFast(_ray, rootId, _rayLength);
#endif
}

bool evalShadow(uint _lightIndex, vec3 _pos)
{
	switch(g_BvhLightData[_lightIndex].iparam)
	{
		case Light_Sphere:
		{
			SphereLight sl = loadSphereLight(_lightIndex);
			vec3 dir = sl.pos - _pos;
			float rayLength = length(dir);
			float d = length(sl.pos - _pos);

			if(rayLength < sl.radius)
				return traverseForShadow(createShadowRay(_pos, dir  / rayLength), rayLength);
			else
				return true;
		}
		//case Light_Area:
		//return evalAreaLight(_rootId, loadAreaLight(_lightIndex), g_BvhMaterialData[_matId], _texColor, _pos, _normal, _eye);
	}

	return false;
}

uvec2 unpackLightCountAndOffset(uint _packed)
{
	uvec2 countOffset;
	countOffset.y = NodeTriangleStride * (_packed & TriangleBitMask);
	countOffset.y += (_packed >> TriangleBitCount) & BlasBitMask;
	countOffset.y += (_packed >> (TriangleBitCount + BlasBitCount)) & PrimitiveBitMask;
	countOffset.x = (_packed >> (TriangleBitCount + BlasBitCount + PrimitiveBitCount)) & LightBitMask;
	return countOffset;
}

uint computeShadowMask(vec3 _pos, uint _nid, in SunDirColor _sun)
{
	uint mask = 0;

#if USE_SHADOW && !COMPUTE_SHADOW_ON_THE_FLY
	Ray sunRay = createShadowRay(_pos, -_sun.sunDir);
	mask |= traverseForShadow(sunRay, TMAX) ? 1 : 0;
	
	uint leafDataOffset = g_BvhNodeData[_nid].nid.w;
	uvec2 lightCountOffset = unpackLightCountAndOffset(g_BvhLeafData[leafDataOffset]);
	lightCountOffset.y += (1 + leafDataOffset);

	for(uint i=0 ; i<lightCountOffset.x ; ++i)
	{
		uint lightIndex = g_BvhLeafData[lightCountOffset.y + i];
		mask |= evalShadow(lightIndex, _pos) ? (1u << (lightIndex + 1)) : 0;
	}
#endif

	return mask;
}

uint computeProbMask(vec3 _pos, in LightProbFieldHeader _lpfHeader)
{
	uint mask = 0;

#if USE_LPF_MASK
	vec3 uvw = getLightProbCoordUVW(_pos, _lpfHeader.resolution, _lpfHeader.aabb, _lpfHeader.step);
	ivec3 coord = ivec3(min(int(uvw.x), _lpfHeader.resolution.x-2), min(int(uvw.y), _lpfHeader.resolution.y-2), min(int(uvw.z), _lpfHeader.resolution.z-2));
	
	for(uint i=0 ; i<8 ; ++i)
	{
		ivec3 offset = ivec3(i & 1, (i >> 1) & 1, (i >> 2) & 1);
		vec3 probPos = getLightProbPosition(coord + offset, _lpfHeader.resolution, _lpfHeader.aabb);

		Ray ray = createShadowRay(_pos, probPos - _pos);
		mask |= ( (traverseForShadow(ray, 1) ? 1u : 0u) << i );
	}
#endif

	return mask;
}

#endif