#ifndef H_BVH_COLLISION_FXH_
#define H_BVH_COLLISION_FXH_

#include "collision.glsl"

Sphere loadSphere(uint objIndex)
{
	Sphere sphere;
	sphere.center = vec3(g_BvhPrimitiveData[objIndex].fparam[0], g_BvhPrimitiveData[objIndex].fparam[1], g_BvhPrimitiveData[objIndex].fparam[2]);
	sphere.radius = g_BvhPrimitiveData[objIndex].fparam[3];
	sphere.invRadius = g_BvhPrimitiveData[objIndex].fparam[4];
	return sphere;
}

Box loadBox(uint objIndex)
{
	Box box;
	box.minExtent = vec3(g_BvhPrimitiveData[objIndex].fparam[0], g_BvhPrimitiveData[objIndex].fparam[1], g_BvhPrimitiveData[objIndex].fparam[2]);
	box.maxExtent = vec3(g_BvhPrimitiveData[objIndex].fparam[3], g_BvhPrimitiveData[objIndex].fparam[4], g_BvhPrimitiveData[objIndex].fparam[5]);
	return box;
}

SphereLight loadSphereLight(uint lightIndex)
{
	SphereLight l;
	l.pos = vec3(g_BvhLightData[lightIndex].fparam[0], g_BvhLightData[lightIndex].fparam[1], g_BvhLightData[lightIndex].fparam[2]);
	l.radius = g_BvhLightData[lightIndex].fparam[3];
	l.color = vec3(g_BvhLightData[lightIndex].fparam[4], g_BvhLightData[lightIndex].fparam[5], g_BvhLightData[lightIndex].fparam[6]);
	l.sphereRadius = g_BvhLightData[lightIndex].fparam[7];
	return l;
}

Sphere loadSphereFromSphereLight(uint lightIndex)
{
	Sphere sphere;
	sphere.center = vec3(g_BvhLightData[lightIndex].fparam[0], g_BvhLightData[lightIndex].fparam[1], g_BvhLightData[lightIndex].fparam[2]);
	sphere.radius = g_BvhLightData[lightIndex].fparam[3];
	sphere.invRadius = 1.0 / sphere.radius;
	return sphere;
}

AreaLight loadAreaLight(uint lightIndex)
{
	AreaLight l;
	l.pos = vec3(g_BvhLightData[lightIndex].fparam[0], g_BvhLightData[lightIndex].fparam[1], g_BvhLightData[lightIndex].fparam[2]);
	l.width = vec3(g_BvhLightData[lightIndex].fparam[3], g_BvhLightData[lightIndex].fparam[4], g_BvhLightData[lightIndex].fparam[5]);
	l.height = vec3(g_BvhLightData[lightIndex].fparam[6], g_BvhLightData[lightIndex].fparam[7], g_BvhLightData[lightIndex].fparam[8]);
	l.color = vec3(g_BvhLightData[lightIndex].fparam[9], g_BvhLightData[lightIndex].fparam[10], g_BvhLightData[lightIndex].fparam[11]);
	l.attenuationRadius = g_BvhLightData[lightIndex].fparam[12];
	return l;
}

void loadTriangleVertices(out vec3 p0, out vec3 p1, out vec3 p2, Triangle triangle)
{
	uint v0Offset = triangle.vertexOffset + (triangle.index01 & 0x0000FFFF);
	p0 = vec3(g_positionData[v0Offset * 3], g_positionData[v0Offset * 3 + 1], g_positionData[v0Offset * 3 + 2]);

	uint v1Offset = triangle.vertexOffset + ((triangle.index01 & 0xFFFF0000) >> 16);
	p1 = vec3(g_positionData[v1Offset * 3], g_positionData[v1Offset * 3 + 1], g_positionData[v1Offset * 3 + 2]);

	uint v2Offset = triangle.vertexOffset + (triangle.index2_matId & 0x0000FFFF);
	p2 = vec3(g_positionData[v2Offset * 3], g_positionData[v2Offset * 3 + 1], g_positionData[v2Offset * 3 + 2]);
}

void loadNormalVertices(out vec3 n0, out vec3 n1, out vec3 n2, Triangle triangle)
{
	uint v0Offset = triangle.vertexOffset + (triangle.index01 & 0x0000FFFF);
	n0 = vec3(g_normalData[v0Offset * 3], g_normalData[v0Offset * 3 + 1], g_normalData[v0Offset * 3 + 2]);

	uint v1Offset = triangle.vertexOffset + ((triangle.index01 & 0xFFFF0000) >> 16);
	n1 = vec3(g_normalData[v1Offset * 3], g_normalData[v1Offset * 3 + 1], g_normalData[v1Offset * 3 + 2]);

	uint v2Offset = triangle.vertexOffset + (triangle.index2_matId & 0x0000FFFF);
	n2 = vec3(g_normalData[v2Offset * 3], g_normalData[v2Offset * 3 + 1], g_normalData[v2Offset * 3 + 2]);
}

void loadUvVertices(out vec2 uv0, out vec2 uv1, out vec2 uv2, Triangle triangle)
{
	uint v0Offset = triangle.vertexOffset + (triangle.index01 & 0x0000FFFF);
	uv0 = vec2(g_texCoordData[v0Offset * 2], g_texCoordData[v0Offset * 2 + 1]);

	uint v1Offset = triangle.vertexOffset + ((triangle.index01 & 0xFFFF0000) >> 16);
	uv1 = vec2(g_texCoordData[v1Offset * 2], g_texCoordData[v1Offset * 2 + 1]);

	uint v2Offset = triangle.vertexOffset + (triangle.index2_matId & 0x0000FFFF);
	uv2 = vec2(g_texCoordData[v2Offset * 2], g_texCoordData[v2Offset * 2 + 1]);
}

bool HitTriangle(Ray r, Triangle triangle, float tMin, float tmax, out Hit outHit)
{
	vec3 p0,p1,p2; 
	loadTriangleVertices(p0, p1, p2, triangle);

	float t = CollideTriangle(r, p0, p1, p2, tmax);
	if(t > 0)
	{
		outHit.t = t;

		vec3 colP = r.from + r.dir * t;
		vec3 v0 = p1 - p0, v1 = p2 - p0, v2 = colP - p0;
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
		loadNormalVertices(n0, n1, n2, triangle);
		outHit.normal = n0 * u + n1 * v + n2 * w;

		vec2 uv0,uv1,uv2; 
		loadUvVertices(uv0, uv1, uv2, triangle);
		outHit.uv = uv0 * u + uv1 * v + uv2 * w;
		return true;
	}
	return false;
}

bool hitPrimitive(uint objIndex, Ray r, float tmax, out Hit hit)
{
	uint type =  g_BvhPrimitiveData[objIndex].iparam & 0xFFFF;

	switch(type)
	{
		case Primitive_Sphere: 
		return HitSphere(r, loadSphere(objIndex), 0, tmax, hit);

		case Primitive_AABB: 
		return HitBox(r, loadBox(objIndex), 0, tmax, hit);
	}

	return false;
}

bool hitPrimitiveThrough(uint objIndex, Ray r, float tmax, out Hit hit)
{
	uint type =  g_BvhPrimitiveData[objIndex].iparam & 0xFFFF;

	switch(type)
	{
		case Primitive_Sphere: 
		return HitSphereThrough(r, loadSphere(objIndex), 0, tmax, hit);

		case Primitive_AABB: 
		return HitBoxThrough(r, loadBox(objIndex), 0, tmax, hit);
	}

	return false;
}

bool hitPrimitiveFast(uint objIndex, Ray r, float tmax)
{
	uint type =  g_BvhPrimitiveData[objIndex].iparam & 0xFFFF;
	Hit hit;
	switch(type)
	{
		case Primitive_Sphere: 
		return CollideSphere(r, loadSphere(objIndex), 0, tmax);

		case Primitive_AABB: 
		return CollideBox(r, loadBox(objIndex), 0, tmax, true) >= 0;
	}

	return false;
}

uvec4 unpackObjectCount(uint _packed)
{
	uvec4 v;
	v.x = _packed & ((1 << TriangleBitCount) - 1);
	v.y = (_packed >> TriangleBitCount) & ((1 << BlasBitCount) - 1);
	v.z = (_packed >> (TriangleBitCount + BlasBitCount)) & ((1 << PrimitiveBitCount) - 1);
	v.w = (_packed >> (TriangleBitCount + BlasBitCount + PrimitiveBitCount)) & ((1 << LightBitCount) - 1);
	return v;
}

uint tlas_collide(uint _nid, Ray _ray, inout ClosestHit closestHit)
{
	_nid = _nid & NID_MASK;
	uint leafDataOffset = g_BvhNodeData[_nid].nid.w;
	if(leafDataOffset == 0xFFFFffff)
		return 0; // early out if empty node

	uvec4 unpackedLeafDat = unpackObjectCount(g_BvhLeafData[leafDataOffset]);
	uint numTriangles = unpackedLeafDat.x;
	uint numBlas = unpackedLeafDat.y;
	uint numObjects = unpackedLeafDat.z;

	uint numTraversal = 0;
	for (uint i = 0; i < numObjects; ++i)
	{
		uint objIndex = g_BvhLeafData[1 + leafDataOffset + numTriangles + numBlas + i];
		Hit hit;
		bool hasHit = hitPrimitive(objIndex, _ray, closestHit.t, hit);

		if (hasHit)
		{
			closestHit.t = hit.t * OFFSET_RAY_COLLISION;
			closestHit.mid_objId = objIndex + (g_BvhPrimitiveData[objIndex].iparam & 0xFFFF0000);
			closestHit.nid = _nid;
			ClosestHit_setDebugColorId(closestHit, objIndex);

			storeHitNormal(closestHit, hit.normal);
		}
	}

	for (uint i = 0; i < numBlas; ++i)
	{
		uint blasIndex = g_BvhLeafData[1 + leafDataOffset + numTriangles + i];
	
		Hit hit;
		Box box = { g_blasHeader[blasIndex].minExtent, g_blasHeader[blasIndex].maxExtent };
		if (HitBox(_ray, box, 0, closestHit.t, hit))
		{
			uint prevNid = closestHit.nid;
			numTraversal += traverseBvh(_ray, g_blasHeader[blasIndex].rootIndex, closestHit);
		}
	}

	return numTraversal;
}

void bvh_collide(uint _nid, Ray _ray, inout ClosestHit closestHit)
{
	_nid = _nid & NID_MASK;
	uint leafDataOffset = g_BvhNodeData[_nid].nid.w;
	uvec4 unpackedLeafDat = unpackObjectCount(g_BvhLeafData[leafDataOffset]);
	uint numTriangles = unpackedLeafDat.x;
	uint numBlas = unpackedLeafDat.y;
	uint numObjects = unpackedLeafDat.z;

	#if !USE_TRAVERSE_TLAS
	for(uint i=0 ; i<numObjects ; ++i)
	{
		uint objIndex = g_BvhLeafData[1 + leafDataOffset + numTriangles + numBlas + i];
		Hit hit;
		bool hasHit = hitPrimitive(objIndex, _ray, closestHit.t, hit);

		if(hasHit)
		{
			closestHit.t			= hit.t * OFFSET_RAY_COLLISION;
			closestHit.mid_objId	= objIndex + (g_BvhPrimitiveData[objIndex].iparam & 0xFFFF0000);
			closestHit.nid			= _nid;
			ClosestHit_setDebugColorId(closestHit, objIndex);

			storeHitNormal(closestHit, hit.normal);
		}
	}
	#endif

	for(uint i=0 ; i<numTriangles ; ++i)
	{
		uint triIndex = g_BvhLeafData[1 + leafDataOffset + i];
		Triangle triangle = g_BvhTriangleData[triIndex];
		Hit hit;
		bool hasHit = HitTriangle(_ray, triangle, 0, closestHit.t, hit);

		if(hasHit)
		{
			vec4 color = vec4(1,1,1,1);
			uint matId = (triangle.index2_matId & 0xFFFF0000) >> 16;
			
			hasHit = color.a > 0.5;
			if (hasHit)
			{
				closestHit.t = hit.t * OFFSET_RAY_COLLISION;
				closestHit.mid_objId = 0x0000FFFF + (triangle.index2_matId & 0xFFFF0000);
				closestHit.nid = _nid;
				ClosestHit_setDebugColorId(closestHit, triIndex);

				storeHitNormal(closestHit, hit.normal);
				storeHitUv(closestHit, hit.uv);
			}
		}
	}

	#if !USE_TRAVERSE_TLAS
	for (uint i = 0; i < numBlas; ++i)
	{
		uint blasIndex = g_BvhLeafData[1 + leafDataOffset + numTriangles + i];

		Hit hit;
		Box box = { g_blasHeader[blasIndex].minExtent, g_blasHeader[blasIndex].maxExtent };
		bool hasHit = !isPointInBox(box, _ray.from) && HitBox(_ray, box, 0, closestHit.t, hit);

		if (hasHit)
		{
			closestHit.t = hit.t * OFFSET_RAY_COLLISION;
			closestHit.mid_objId = 0x0000FFFF + (g_blasHeader[blasIndex].matId & 0xFFFF0000);
			closestHit.nid = _nid;
			ClosestHit_setDebugColorId(closestHit, blasIndex);

			storeHitNormal(closestHit, hit.normal);
		}
	}
	#endif
}

bool bvh_collide_fast(uint _nid, Ray _ray, float tmax)
{
	_nid = _nid & NID_MASK;
	uint leafDataOffset = g_BvhNodeData[_nid].nid.w;
	uvec4 unpackedLeafDat = unpackObjectCount(g_BvhLeafData[leafDataOffset]);
	uint numTriangles = unpackedLeafDat.x;
	uint numBlas = unpackedLeafDat.y;
	uint numObjects = unpackedLeafDat.z;

	#if !USE_TRAVERSE_TLAS
	for(uint i=0 ; i<numObjects ; ++i)
	{
		uint objIndex = g_BvhLeafData[1 + leafDataOffset + numTriangles + numBlas + i];
		if(hitPrimitiveFast(objIndex, _ray, tmax))
			return true;
	}
	#endif

	for(uint i=0 ; i<numTriangles ; ++i)
	{
		uint triIndex = g_BvhLeafData[1 + leafDataOffset + i];
		vec3 p0,p1,p2; 
		loadTriangleVertices(p0, p1, p2, g_BvhTriangleData[triIndex]);

		if(CollideTriangle(_ray, p0, p1, p2, tmax) > 0)
			return true;
	}

	#if !USE_TRAVERSE_TLAS
	for (uint i = 0; i < numBlas; ++i)
	{
		uint blasIndex = g_BvhLeafData[1 + leafDataOffset + numTriangles + i];

		Box box = { g_blasHeader[blasIndex].minExtent, g_blasHeader[blasIndex].maxExtent };

		if (!isPointInBox(box, _ray.from))
		{
			if (CollideBox(_ray, box, 0, tmax, true) >= 0)
				return true;
		}
	}
	#endif

	return false;
}

bool tlas_collide_fast(uint _nid, Ray _ray, float tmax)
{
	uint numTraversal = 0;

	_nid = _nid & NID_MASK;
	uint leafDataOffset = g_BvhNodeData[_nid].nid.w;
	if(leafDataOffset == 0xFFFFffff)
		return false; // early out if empty node

	uvec4 unpackedLeafDat = unpackObjectCount(g_BvhLeafData[leafDataOffset]);
	uint numTriangles = unpackedLeafDat.x;
	uint numBlas = unpackedLeafDat.y;
	uint numObjects = unpackedLeafDat.z;

	for(uint i=0 ; i<numObjects ; ++i)
	{
		uint objIndex = g_BvhLeafData[1 + leafDataOffset + numTriangles + numBlas + i];
		if(hitPrimitiveFast(objIndex, _ray, tmax))
			return true;
	}

	for (uint i = 0; i < numBlas; ++i)
	{
		uint blasIndex = g_BvhLeafData[1 + leafDataOffset + numTriangles + i];
	
		Box box = { g_blasHeader[blasIndex].minExtent, g_blasHeader[blasIndex].maxExtent };	
		if (CollideBox(_ray, box, 0, tmax, true) >= 0)
		{
			if(traverseBvhFast(_ray, g_blasHeader[blasIndex].rootIndex, tmax))
				return true;
		}
	}

	return false;
}

#if NO_BVH
void brutForceTraverse(Ray _ray, inout ClosestHit closestHit)
{
	closestHit.nid = 0xFFFFffff;
	uint debugObjIndex = 0;
	for(uint i=0 ; i<g_Constants.numPrimitives ; ++i)
	{
		Hit hit;
		bool hasHit = hitPrimitive(i, _ray, closestHit.t, hit);

		closestHit.t =			hasHit ? hit.t * OFFSET_RAY_COLLISION	: closestHit.t;
		closestHit.normal =		hasHit ? hit.normal						: closestHit.normal;
		closestHit.mid_objId =	hasHit ? (i | (g_BvhPrimitiveData[i].iparam & 0xFFFF0000)) : closestHit.mid_objId;

		debugObjIndex++;
		if(hasHit)
			ClosestHit_setDebugColorId(closestHit, debugObjIndex);
	}

	for(uint i=0 ; i<g_Constants.numTriangles ; ++i)
	{
		Hit hit;
		bool hasHit = HitTriangle(_ray, g_BvhTriangleData[i], 0, closestHit.t, hit);

		closestHit.t =			hasHit ? hit.t * OFFSET_RAY_COLLISION		: closestHit.t;
		closestHit.normal =		hasHit ? hit.normal							: closestHit.normal;
		closestHit.uv =			hasHit ? hit.uv								: closestHit.uv;
		closestHit.mid_objId =	hasHit ? 0x0000FFFF | (g_BvhTriangleData[i].index2_matId & 0xFFFF0000) : closestHit.mid_objId;

		debugObjIndex++;
		if (hasHit)
			ClosestHit_setDebugColorId(closestHit, debugObjIndex);
	}

	for (uint i = 0; i < g_Constants.numBlas; ++i)
	{
		Hit hit;

		Box box = { g_blasHeader[i].minExtent, g_blasHeader[i].maxExtent };
		bool hasHit = !isPointInBox(box, _ray.from) && HitBox(_ray, box, 0, closestHit.t, hit);
	
		closestHit.t = hasHit ? hit.t * OFFSET_RAY_COLLISION : closestHit.t;
		closestHit.normal = hasHit ? hit.normal : closestHit.normal;
		closestHit.mid_objId = hasHit ? (0x0000FFFF | ((g_blasHeader[i].matId << 16) & 0xFFFF0000)) : closestHit.mid_objId;

		debugObjIndex++;
		if(hasHit)
			ClosestHit_setDebugColorId(closestHit, debugObjIndex);
	}
}
#endif

#endif