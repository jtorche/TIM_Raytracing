#ifndef H_BASERAYTRACINGPASS_FXH_
#define H_BASERAYTRACINGPASS_FXH_

#include "struct_cpp.glsl"
#include "core/collision.glsl"
#include "core/lighting.glsl"

#include "bvh/bvhBindings.glsl"
#include "bvh/bvhTraversal.glsl"
#include "bvh/bvhGetter.glsl"
#include "bvh/bvhCollision.glsl"

#include "bvh/bvhLighting.glsl"

//--------------------------------------------------------------------------------
uint rayTrace(in Ray _ray, out ClosestHit _hitResult)
{
	_hitResult.t = TMAX;
	_hitResult.mid_objId = 0xFFFFffff;
	_hitResult.nid = 0xFFFFffff;
	ClosestHit_setDebugColorId(_hitResult, 0);
	storeHitUv(_hitResult, vec2(0,0));

	uint rootId = g_Constants.numNodes == 1 ? NID_LEAF_BIT : 0;

#ifdef USE_TRAVERSE_TLAS
	return traverseTlas(_ray, rootId, _hitResult);
#else
	return traverseBvh(_ray, rootId, _hitResult);
#endif
}

//--------------------------------------------------------------------------------
vec3 computeLighting(in SunDirColor _sun, in LightProbFieldHeader _lpfHeader, in Ray _ray, in ClosestHit _closestHit)
{
	if(_closestHit.t < TMAX)
	{
	#if NO_LIGHTING
		return g_BvhMaterialData[getMaterialId(_closestHit)].color.xyz;
	#else
		uint rootId = g_Constants.numNodes == 1 ? NID_LEAF_BIT : 0;
		return computeLighting(rootId, _ray, _sun, _lpfHeader, _closestHit);
	#endif
	}
	else
		return vec3(0,0,0);
}

//--------------------------------------------------------------------------------
vec3 applyDebugLighting(in Ray _ray, in ClosestHit _closestHit, uint _numTraversal)
{
#if DEBUG_BVH_TRAVERSAL
	const uint numColors = 5;
	const uint step = 50;
	const uint maxTraversal = step * numColors;
	vec3 dbgColor[6] =
	{
		vec3(0,1,0),
		vec3(0,1,1),
		vec3(0,0,1),
		vec3(1,0,1),
		vec3(1,0,0),
		vec3(1,0,0)
	};

	_numTraversal = min(_numTraversal, maxTraversal);

	for (uint i = 0; i < numColors; ++i)
	{
		if (_numTraversal <= step + step * i)
		{
			return mix(dbgColor[i], dbgColor[i+1], float(_numTraversal - step * i) / step);
		}
	}
	return dbgColor[numColors - 1];
#endif
	
#if DEBUG_GEOMETRY || DEBUG_BVH
	if(_closestHit.t < TMAX)
	{
		vec3 dbgColor[14] = 
		{
			vec3(1,0,0),
			vec3(0,1,0),
			vec3(0,0,1),
			vec3(1,1,0),
			vec3(1,0,1),
			vec3(0,1,1),
			
			vec3(1,0.5,0),
			vec3(1,0,0.5),
			vec3(0,1,0.5),
			vec3(0.5,1,0),
			vec3(0.5,0,1),
			vec3(0,0.5,1),
			
			vec3(0.3,0.3,0.3),
			vec3(0.7,0.7,0.7)
		};
	#if DEBUG_GEOMETRY
		return dbgColor[_closestHit.dbgColorId % 14];
	#else
		return dbgColor[_closestHit.nid % 14];
	#endif
	}
#endif

	return vec3(0,0,0);
}

#endif