#ifndef H_BVH_COLLISION_FXH_
#define H_BVH_COLLISION_FXH_

#include "core/collision.glsl"

#define INLINE_TRIANGLE 1
#define INLINE_STRIP 0

#if INLINE_STRIP
	#define NodeTriangleStride 4
	
	TriangleStrip loadTriangleStripFromNode(uint _offset)
	{
		TriangleStrip tri;
		tri.vertexOffset = g_BvhLeafData[_offset];
		tri.index01 = g_BvhLeafData[_offset+1];
		tri.index2_matId = g_BvhLeafData[_offset+2];
		tri.index34 = g_BvhLeafData[_offset+3];
		return tri;
	}
#elif INLINE_TRIANGLE
	#define NodeTriangleStride 3
	
	Triangle loadTriangleFromNode(uint _offset)
	{
		Triangle tri;
		tri.vertexOffset = g_BvhLeafData[_offset];
		tri.index01 = g_BvhLeafData[_offset+1];
		tri.index2_matId = g_BvhLeafData[_offset+2];
		return tri;
	}

#else
	#define NodeTriangleStride 1

	Triangle loadTriangleFromNode(uint _offset)
	{
		uint triIndex = g_BvhLeafData[_offset];
		return g_BvhTriangleData[triIndex];
	}
#endif

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

bool HitTriangle(Ray r, Triangle triangle, float tmax, out Hit outHit)
{
	vec3 p0,p1,p2; 
	loadTriangleVertices(p0, p1, p2, getIndex0(triangle), getIndex1(triangle), getIndex2(triangle));
	outHit.t = CollideTriangle(r, p0, p1, p2, tmax);
	
	return outHit.t > 0;
}

//bool HitTriangleStrip(Ray r, TriangleStrip strip, float tMin, float tmax, out Hit outHit)
//{
//	uint offset = strip.vertexOffset + (strip.index01 & 0x0000FFFF);
//	vec3 p0 = vec3(g_positionData[offset * 3], g_positionData[offset * 3 + 1], g_positionData[offset * 3 + 2]);
//	
//	offset = strip.vertexOffset + ((strip.index01 & 0xFFFF0000) >> 16);
//	vec3 p1 = vec3(g_positionData[offset * 3], g_positionData[offset * 3 + 1], g_positionData[offset * 3 + 2]);
//	
//	offset = strip.vertexOffset + (strip.index2_matId & 0x0000FFFF);
//	vec3 p2 = vec3(g_positionData[offset * 3], g_positionData[offset * 3 + 1], g_positionData[offset * 3 + 2]);
//	
//	float t0 = CollideTriangle(r, p0, p1, p2, tmax);
//	float t = tmax;
//	if(t0 > 0)
//	{
//		t = t0;
//		fillUvNormal(r, t, p0,p1,p2, getIndex0(strip), getIndex1(strip), getIndex2(strip), outHit);
//	}
//
//	offset = strip.index34 & 0x0000FFFF;
//	if(offset != 0xFFFF)
//	{
//		offset += strip.vertexOffset;
//		vec3 p3 = vec3(g_positionData[offset * 3], g_positionData[offset * 3 + 1], g_positionData[offset * 3 + 2]);
//
//		float t1 = CollideTriangle(r, p1, p2, p3, tmax);
//		if(t1 > 0 && t1 < t)
//		{
//			t = t1;
//			fillUvNormal(r, t, p1,p2,p3, getIndex1(strip), getIndex2(strip), getIndex3(strip), outHit);
//		}
//
//		offset = (strip.index34 & 0xFFFF0000) >> 16;
//		if(offset != 0xFFFF)
//		{
//			offset += strip.vertexOffset;
//			vec3 p4 = vec3(g_positionData[offset * 3], g_positionData[offset * 3 + 1], g_positionData[offset * 3 + 2]);
//		
//			float t2 = CollideTriangle(r, p2, p3, p4, tmax);
//			if(t2 > 0 && t2 < t)
//			{
//				t = t2;
//				fillUvNormal(r, t, p2,p3,p4, getIndex2(strip), getIndex3(strip), getIndex4(strip), outHit);
//			}
//		}
//	}
//
//	return t < tmax;
//}

//bool hitTriangleStripFast(Ray r, in TriangleStrip strip, float tmax)
//{
//	uint offset = strip.vertexOffset + (strip.index01 & 0x0000FFFF);
//	vec3 p0 = vec3(g_positionData[offset * 3], g_positionData[offset * 3 + 1], g_positionData[offset * 3 + 2]);
//
//	offset = strip.vertexOffset + ((strip.index01 & 0xFFFF0000) >> 16);
//	vec3 p1 = vec3(g_positionData[offset * 3], g_positionData[offset * 3 + 1], g_positionData[offset * 3 + 2]);
//
//	offset = strip.vertexOffset + (strip.index2_matId & 0x0000FFFF);
//	vec3 p2 = vec3(g_positionData[offset * 3], g_positionData[offset * 3 + 1], g_positionData[offset * 3 + 2]);
//
//	if(CollideTriangle(r, p0, p1, p2, tmax) > 0)
//		return true;
//
//	offset = strip.index34 & 0x0000FFFF;
//	if(offset != 0xFFFF)
//	{
//		offset += strip.vertexOffset;
//		vec3 p3 = vec3(g_positionData[offset * 3], g_positionData[offset * 3 + 1], g_positionData[offset * 3 + 2]);
//	
//		if(CollideTriangle(r, p1, p2, p3, tmax) > 0)
//			return true;
//	
//		offset = (strip.index34 & 0xFFFF0000) >> 16;
//		if(offset != 0xFFFF)
//		{
//			offset += strip.vertexOffset;
//			vec3 p4 = vec3(g_positionData[offset * 3], g_positionData[offset * 3 + 1], g_positionData[offset * 3 + 2]);
//		
//			if(CollideTriangle(r, p2, p3, p4, tmax) > 0)
//				return true;
//		}
//	}
//
//	return false;
//}

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
	uint triangleOffset = unpackedLeafDat.x * NodeTriangleStride;
	uint numBlas = unpackedLeafDat.y;

	uint numTraversal = 0;

	uint prevNid = closestHit.nid;
	for (uint i = 0; i < numBlas; ++i)
	{
		uint blasIndex = g_BvhLeafData[1 + leafDataOffset + triangleOffset + i];
	
		Hit hit;
		Box box = { g_blasHeader[blasIndex].minExtent, g_blasHeader[blasIndex].maxExtent };
		if (HitBox(_ray, box, closestHit.t, hit))
		{
			uint prevNid = closestHit.nid;
			numTraversal += traverseBvh(_ray, g_blasHeader[blasIndex].rootIndex, closestHit);
				
		}
	}

	if(closestHit.nid != prevNid) // find collision with blas
		closestHit.nid = _nid;

	return numTraversal;
}

void bvh_collide(uint _nid, Ray _ray, inout ClosestHit _closestHit)
{
	_nid = _nid & NID_MASK;
	uint leafDataOffset = g_BvhNodeData[_nid].nid.w;
	uvec4 unpackedLeafDat = unpackObjectCount(g_BvhLeafData[leafDataOffset]);
	uint numTriangles = unpackedLeafDat.x;
	uint triangleOffset = numTriangles * NodeTriangleStride;
	uint numBlas = unpackedLeafDat.y;

	#if INLINE_STRIP
	for(uint i=0 ; i<numTriangles ; ++i)
	{
		TriangleStrip strip = loadTriangleStripFromNode(1 + leafDataOffset + i * NodeTriangleStride);
		Hit hit;
		bool hasHit = HitTriangleStrip(_ray, strip, 0, _closestHit.t, hit);
		
		if(hasHit)
		{
			_closestHit.t = hit.t - OFFSET_RAY_COLLISION;

			ClosestHit_storeTriangle(_closestHit, triangle);
			ClosestHit_storeNid(_closestHit, _nid);
			ClosestHit_setDebugColorId(_closestHit, triangle.index01+triangle.index2_matId);
		}
	}
	#else
	for(uint i=0 ; i<numTriangles ; ++i)
	{
		Triangle triangle = loadTriangleFromNode(1 + leafDataOffset + i * NodeTriangleStride);
		Hit hit;
		bool hasHit = HitTriangle(_ray, triangle, _closestHit.t, hit);

		if(hasHit)
		{
			_closestHit.t = hit.t - OFFSET_RAY_COLLISION;

			ClosestHit_storeTriangle(_closestHit, triangle);
			ClosestHit_storeNid(_closestHit, _nid);
			ClosestHit_setDebugColorId(_closestHit, triangle.index01+triangle.index2_matId);
		}
	}
	#endif

	#ifndef USE_TRAVERSE_TLAS
	for (uint i = 0; i < numBlas; ++i)
	{
		uint blasIndex = g_BvhLeafData[1 + leafDataOffset + triangleOffset + i];

		Hit hit;
		Box box = { g_blasHeader[blasIndex].minExtent, g_blasHeader[blasIndex].maxExtent };
		bool hasHit = !isPointInBox(box, _ray.from) && HitBox(_ray, box, _closestHit.t, hit);

		if (hasHit)
		{
			_closestHit.t = hit.t - OFFSET_RAY_COLLISION;

			ClosestHit_storeNid(_closestHit, _nid);
			ClosestHit_setDebugColorId(_closestHit, blasIndex);
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
	uint triangleOffset = numTriangles * NodeTriangleStride;
	uint numBlas = unpackedLeafDat.y;

	#if INLINE_STRIP
	for(uint i=0 ; i<numTriangles ; ++i)
	{
		if(hitTriangleStripFast(_ray, loadTriangleStripFromNode(1 + leafDataOffset + i * NodeTriangleStride), tmax))
			return true;
	}
	#else
	for(uint i=0 ; i<numTriangles ; ++i)
	{
		Triangle triangle = loadTriangleFromNode(1 + leafDataOffset + i * NodeTriangleStride);
		vec3 p0,p1,p2; 
		loadTriangleVertices(p0, p1, p2, getIndex0(triangle), getIndex1(triangle), getIndex2(triangle));

		if(CollideTriangle(_ray, p0, p1, p2, tmax) > 0)
			return true;
	}
	#endif

	#ifndef USE_TRAVERSE_TLAS
	for (uint i = 0; i < numBlas; ++i)
	{
		uint blasIndex = g_BvhLeafData[1 + leafDataOffset + triangleOffset + i];

		Box box = { g_blasHeader[blasIndex].minExtent, g_blasHeader[blasIndex].maxExtent };

		if (!isPointInBox(box, _ray.from))
		{
			if (CollideBox(_ray, box, tmax, true) > 0)
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
	uint triangleOffset = numTriangles * NodeTriangleStride;
	uint numBlas = unpackedLeafDat.y;

	for (uint i = 0; i < numBlas; ++i)
	{
		uint blasIndex = g_BvhLeafData[1 + leafDataOffset + triangleOffset + i];
	
		Box box = { g_blasHeader[blasIndex].minExtent, g_blasHeader[blasIndex].maxExtent };	
		if (CollideBox(_ray, box, tmax, true) > 0)
		{
			if(traverseBvhFast(_ray, g_blasHeader[blasIndex].rootIndex, tmax))
				return true;
		}
	}

	return false;
}

#endif