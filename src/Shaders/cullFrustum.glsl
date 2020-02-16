#ifndef H_CULLFRUSTUM_FXH_
#define H_CULLFRUSTUM_FXH_

#include "struct_cpp.glsl"

#if TILE_FRUSTUM_CULL

vec4 computePlan(vec3 p1, vec3 p2, vec3 p3)
{
	vec3 n=normalize(cross(p1 - p2, p3 - p2));
	return vec4(n, -dot(n,p2));
}

bool primitiveFrustumCollision(uint objIndex, in vec4 _plans[4])
{
	uint type =  g_BvhPrimitiveData[objIndex].iparam & 0xFFFF;
	switch(type)
	{
		case Primitive_Sphere: 
		return sphereFrustum4Collision(loadSphere(objIndex), _plans);

		case Primitive_AABB: 
		return boxFrustum4Collision(loadBox(objIndex), _plans);
	}

	return false;
}

bool lightFrustumCollision(uint lightIndex, in vec4 _plans[4])
{
	return sphereFrustum4Collision(loadSphereFromSphereLight(lightIndex), _plans);
}

#define MAX_PRIMITIVES_PER_TILE LOCAL_SIZE * LOCAL_SIZE
#define MAX_LIGHTS_PER_TILE 16
shared uint g_primitives[MAX_PRIMITIVES_PER_TILE];
shared uint g_primitiveCount;
shared uint g_lights[MAX_LIGHTS_PER_TILE];
shared uint g_lightCount;

void cullWithFrustumTile(in PassData _passData, in PushConstants _constants)
{
	vec3 step_x = (_passData.frustumCorner10.xyz - _passData.frustumCorner00.xyz) / gl_NumWorkGroups.x;
	vec3 step_y = (_passData.frustumCorner01.xyz - _passData.frustumCorner00.xyz) / gl_NumWorkGroups.y;
	
	vec3 tile_bl = _passData.frustumCorner00.xyz + step_x * gl_WorkGroupID.x +		step_y * gl_WorkGroupID.y;
	vec3 tile_br = _passData.frustumCorner00.xyz + step_x * (gl_WorkGroupID.x+1) +	step_y * gl_WorkGroupID.y;
	vec3 tile_tl = _passData.frustumCorner00.xyz + step_x * gl_WorkGroupID.x +		step_y * (gl_WorkGroupID.y+1);
	vec3 tile_tr = _passData.frustumCorner00.xyz + step_x * (gl_WorkGroupID.x+1) +	step_y * (gl_WorkGroupID.y+1);
	
	vec4 plans[4]; // left right top down near far
	plans[0] = computePlan(vec3(_passData.cameraPos), tile_tl, tile_bl);
	plans[1] = computePlan(vec3(_passData.cameraPos), tile_br, tile_tr);
	plans[2] = computePlan(vec3(_passData.cameraPos), tile_tr, tile_tl);
	plans[3] = computePlan(vec3(_passData.cameraPos), tile_bl, tile_br);

	//vec4 pts_near = _passData.invProjView*vec4(0,0,0.01,1);
	//vec4 pts_far = _passData.invProjView*vec4(0,0,minMaxDepth.y*2-1,1);
	//plans[4] = computePlan(pts_near.xyz/pts_near.w, cameraDir);
	//plans[5] = computePlan(pts_far.xyz/pts_far.w, -cameraDir.xyz);
	
	if(gl_LocalInvocationIndex == 0)
	{
		g_primitiveCount = 0;
		g_lightCount = 0;
	}
		
	barrier();

	// Cull primitives
	uint localRange = 1 + (_constants.numPrimitives / (LOCAL_SIZE * LOCAL_SIZE));
	uint start = gl_LocalInvocationIndex * localRange;
	uint end = min(start+localRange, _constants.numPrimitives);

	uint safe_i=0;
	for(uint i=start ; i<end && safe_i < MAX_PRIMITIVES_PER_TILE ; ++i)
	{
		if(primitiveFrustumCollision(i, plans))
		{
		    safe_i = atomicAdd(g_primitiveCount, 1);
			g_primitives[safe_i] = i;
		}
	}

	// Cull lights
	localRange = 1 + (_constants.numLights / (LOCAL_SIZE * LOCAL_SIZE));
	start = gl_LocalInvocationIndex * localRange;
	end = min(start+localRange, _constants.numLights);

	safe_i=0;
	for(uint i=start ; i<end && safe_i < MAX_LIGHTS_PER_TILE ; ++i)
	{
		if(lightFrustumCollision(i, plans))
		{
		    safe_i = atomicAdd(g_lightCount, 1);
			g_lights[safe_i] = i;
		}
	}

	barrier();
}

void collideRayAgainstTileData(in Ray _ray, inout ClosestHit _closestHit)
{
	uint matId = 0;
	#if USE_SHARED_MEM
	vec3 normal;
	bool anyHit = false;
	#endif

	for(uint i=0 ; i<g_primitiveCount ; ++i)
	{
		uint primIndex = g_primitives[i];
		Hit hit;
		bool hasHit = hitPrimitive(primIndex, _ray, _closestHit.t, hit);

		_closestHit.t =		 hasHit ? hit.t * 0.999	: _closestHit.t;

		#if USE_SHARED_MEM
		if(hasHit)
		{
			normal = hit.normal;
			anyHit = true;
		}
		#else
		_closestHit.normal = hasHit ? hit.normal : _closestHit.normal;
		#endif	

		matId =				 hasHit ? g_BvhPrimitiveData[primIndex].iparam	: matId;
	}

	_closestHit.nid_mid = (matId & 0xFFFF0000) >> 16;
	#if USE_SHARED_MEM
	if(anyHit)
		g_normalHit[gl_LocalInvocationIndex] = normal;
	#endif
}

vec3 evalLightWithTileData(uint rootId, in Ray _ray, in ClosestHit _closestHit)
{
	vec3 pixelColor = vec3(0,0,0);
	for(uint i=0 ; i<g_lightCount ; ++i)
		pixelColor += evalLighting(rootId, g_lights[i], _closestHit.nid_mid, _ray, _closestHit);

	return pixelColor;
}

#endif

#endif