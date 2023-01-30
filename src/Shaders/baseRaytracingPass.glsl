#ifndef H_BASERAYTRACINGPASS_FXH_
#define H_BASERAYTRACINGPASS_FXH_

#include "struct_cpp.glsl"
#include "collision.glsl"
#include "lighting.glsl"

#include "bvhBindings.glsl"

#include "bvhTraversal.glsl"
#include "bvhGetter.glsl"
#include "bvhCollision.glsl"

#include "bvhLighting.glsl"

vec3 rayTrace(in SunDirColor _sun, in Ray _ray, out ClosestHit _hitResult)
{
	ClosestHit closestHit;
	closestHit.t = TMAX;
	closestHit.mid_objId = 0xFFFFffff;
	closestHit.nid = 0xFFFFffff;
	ClosestHit_setDebugColorId(closestHit, 0);
	storeHitUv(closestHit, vec2(0,0));

	uint rootId = g_Constants.numNodes == 1 ? NID_LEAF_BIT : 0;

#if NO_BVH
	brutForceTraverse(_ray, closestHit);
#else
	#if USE_TRAVERSE_TLAS
	uint numTraversal = traverseTlas(_ray, rootId, closestHit);
	#else
	uint numTraversal = traverseBvh(_ray, rootId, closestHit);
	#endif

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

	numTraversal = min(numTraversal, maxTraversal);

	for (uint i = 0; i < numColors; ++i)
	{
		if (numTraversal <= step + step * i)
			return mix(dbgColor[i], dbgColor[i+1], float(numTraversal - step * i) / step);
	}
	return dbgColor[numColors - 1];
	#endif
#endif
	
	vec3 lit = vec3(0,0,0);
	if(closestHit.t < TMAX)
	{
#if !DEBUG_GEOMETRY && !DEBUG_BVH
	#if NO_LIGHTING
		lit = getHitColor(closestHit);
	#else
		lit += computeDirectLighting(rootId, _ray, _sun, closestHit);
	#endif

#else
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
		   lit = dbgColor[closestHit.dbgColorId % 14];
		   #else
		   lit = dbgColor[closestHit.nid % 14];
		   #endif
#endif
	}

	_hitResult = closestHit;

	return lit;
}

#endif