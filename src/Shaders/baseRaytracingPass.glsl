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

vec3 rayTrace(in PassData _passData, in Ray _ray, out ClosestHit _hitResult)
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
	uint numTraversal = traverseBvh(_ray, rootId, closestHit);
	#if DEBUG_BVH_TRAVERSAL

	const uint numColors = 5;
	const uint step = 150;
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
	#if NO_BVH
		for(uint i=0 ; i<g_Constants.numLights ; ++i)
			lit += evalLighting(rootId, i, getMaterialId(closestHit), _ray, closestHit);
	#else
           lit += computeDirectLighting(rootId, _ray, closestHit);
	#endif

		   lit += computeSunLighting(rootId, _passData.sunDir.xyz, _passData.sunColor.xyz, getMaterialId(closestHit), _ray, closestHit);

#if DEBUG_GEOMETRY
		   vec3 dbgColor[8] = 
		   {
			   vec3(1,0,0),
			   vec3(0,1,0),
			   vec3(0,0,1),
			   vec3(1,1,0),
			   vec3(1,0,1),
			   vec3(0,1,1),
			   vec3(0.3,0.3,0.3),
			   vec3(0.7,0.7,0.7)
		   };

		   lit = dbgColor[closestHit.dbgColorId % 8];
#endif
	}

	_hitResult = closestHit;

	return lit;
}

#endif