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

	uint rootId = g_Constants.numNodes == 1 ? NID_LEAF_BIT : 0;

#ifdef USE_TRAVERSE_TLAS
	return traverseTlas(_ray, rootId, _hitResult);
#else
	return traverseBvh(_ray, rootId, _hitResult);
#endif
}

Triangle unpackTriangle(uvec3 _packed)
{
	Triangle tri;
	tri.vertexOffset = _packed.x;
	tri.index01 = _packed.y;
	tri.index2_matId = _packed.z;
	return tri;
}

void fillUvNormal(vec3 _pos, in Triangle _tri, out vec3 _normal, out vec2 _uv)
{
	uint index0 = _tri.vertexOffset + (_tri.index01 & 0x0000FFFF);
	uint index1 = _tri.vertexOffset + ((_tri.index01 & 0xFFFF0000) >> 16);
	uint index2 = _tri.vertexOffset + (_tri.index2_matId & 0x0000FFFF);
	vec3 p0, p1, p2;
	loadTriangleVertices(p0, p1, p2, index0, index1, index2);

	vec3 v0 = p1 - p0, v1 = p2 - p0, v2 = _pos - p0;
	float d00 = dot(v0, v0);
	float d01 = dot(v0, v1);
	float d11 = dot(v1, v1);
	float d20 = dot(v2, v0);
	float d21 = dot(v2, v1);
	float denom = d00 * d11 - d01 * d01;
	float v = (d11 * d20 - d01 * d21) / denom;
	float w = (d00 * d21 - d01 * d20) / denom;
	float u = 1 - v - w;

	vec3 n0,n1,n2; 
	loadNormalVertices(n0, n1, n2, index0, index1, index2);
	_normal = n0 * u + n1 * v + n2 * w;

	vec2 uv0,uv1,uv2; 
	loadUvVertices(uv0, uv1, uv2, index0, index1, index2);
	_uv = uv0 * u + uv1 * v + uv2 * w;
}

//--------------------------------------------------------------------------------
vec3 computeLighting(in SunDirColor _sun, in LightProbFieldHeader _lpfHeader, uint _shadowMask, in Ray _ray, float _t, in Triangle _triangle)
{
	if(_t < TMAX)
	{
		vec3 pos = _ray.from + _ray.dir * _t;
		vec3 eye = _ray.from;
		vec3 normal; 
		vec2 uv = vec2(0,0);
		fillUvNormal(pos, _triangle, normal, uv);
		
		uint rootId = g_Constants.numNodes == 1 ? NID_LEAF_BIT : 0;
		return computeLighting(rootId, _sun, _lpfHeader, _shadowMask, getTriangleMaterial(_triangle), pos, eye, normal, uv);
	}

	return vec3(200,220,255) / 255;
}

#ifdef TRACING_STEP
uvec2 computeAdditionalHitData(in Ray _ray, in ClosestHit _closestHit, in SunDirColor _sun, in LightProbFieldHeader _lpfHeader)
{	
	uvec2 hitData = uvec2(0, 0xFFFFffff);
	if(_closestHit.t > 0)
	{
		uint nid = ClosestHit_getNid(_closestHit);
		vec3 hitPos = _ray.from + _ray.dir * _closestHit.t;
		hitData.x = computeShadowMask(hitPos, nid, _sun); 
		hitData.y = computeProbMask(hitPos, _lpfHeader);
	}

	return hitData;
}
#endif

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
		return dbgColor[ClosestHit_getNid(_closestHit) % 14];
	#endif
	}
#endif

	return vec3(0,0,0);
}

#endif
