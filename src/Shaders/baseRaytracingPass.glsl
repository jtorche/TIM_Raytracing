#ifndef H_BASERAYTRACINGPASS_FXH_
#define H_BASERAYTRACINGPASS_FXH_

#include "collision.glsl"
#include "lighting.glsl"

#include "bvhBindings.glsl"

#include "bvhTraversal.glsl"
#include "bvhGetter.glsl"
#include "bvhCollision.glsl"

#include "bvhLighting.glsl"

vec3 rayTrace(in Ray _ray, out ClosestHit _hitResult)
{
	ClosestHit closestHit;
	closestHit.t = TMAX;
	closestHit.nid_mid = 0xFFFFffff;
	uint rootId = g_Constants.numNodes == 1 ? 0x8000 : 0;

#if NO_BVH
	brutForceTraverse(_ray, closestHit);
#else
	traverseBvh(_ray, rootId, closestHit);
#endif
	
	vec3 lit = vec3(0,0,0);
	if(closestHit.t < TMAX)
	{
	#if NO_BVH
		for(uint i=0 ; i<g_Constants.numLights ; ++i)
			lit += evalLighting(rootId, i, closestHit.nid_mid, _ray, closestHit);
	#else
           lit = computeDirectLighting(rootId, _ray, closestHit);
	#endif
	}

	_hitResult = closestHit;
	return lit;
}

#endif