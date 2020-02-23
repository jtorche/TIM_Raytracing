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

bool HitTriangle(Ray r, Triangle triangle, float tMin, float tmax, out Hit outHit)
{
	vec3 p0,p1,p2; 
	loadTriangleVertices(p0, p1, p2, triangle);

	float t = CollideTriangle(r, p0, p1, p2, tmax);
	if(t > 0)
	{
		outHit.t = t;
		outHit.normal = vec3(0,0,1);
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

		//case Primitive_Triangle: 
		//vec3 p0, p1, p2;
		//loadTriangleVertices(p0, p1, p2, loadTriangle(objIndex));
		//return CollideTriangle(r, p0, p1, p2, tmax) > 0; 
	}

	return false;
}

uvec3 unpackObjectCount(uint _packed)
{
	uvec3 v;
	v.x = _packed & ((1 << TriangleBitCount) - 1);
	v.y = (_packed >> TriangleBitCount) & ((1 << PrimitiveBitCount) - 1);
	v.z = (_packed >> (TriangleBitCount + PrimitiveBitCount)) & ((1 << LightBitCount) - 1);
	return v;
}

void bvh_collide(uint _nid, Ray _ray, inout ClosestHit closestHit)
{
	_nid = _nid & 0x7FFF;
	uint leafDataOffset = g_BvhNodeData[_nid].nid.z;
	uvec3 unpackedLeafDat = unpackObjectCount(g_BvhNodeData[_nid].nid.w);
	uint numTriangles = unpackedLeafDat.x;
	uint numObjects = unpackedLeafDat.y;
#if USE_SHARED_MEM
	vec3 normal;
	bool anyHit = false;
#endif

	for(uint i=0 ; i<numObjects ; ++i)
	{
		uint objIndex = g_BvhLeafData[numTriangles + leafDataOffset + i];
		Hit hit;
		bool hasHit = hitPrimitive(objIndex, _ray, closestHit.t, hit);

		if(hasHit)
		{
			closestHit.t		= hit.t * 0.999;
			closestHit.nid_mid	= _nid + (g_BvhPrimitiveData[objIndex].iparam & 0xFFFF0000);
			closestHit.objectId = objIndex;
		#if USE_SHARED_MEM
			normal = hit.normal;
			anyHit = true;
		#else
			closestHit.normal	= hit.normal;
		#endif	
		}
	}

	#if USE_SHARED_MEM
	if(anyHit)
		g_normalHit[gl_LocalInvocationIndex] = normal;
	#endif
}

bool bvh_collide_fast(uint _nid, Ray _ray, float tmax)
{
	_nid = _nid & 0x7FFF;
	uint leafDataOffset = g_BvhNodeData[_nid].nid.z;
	uvec3 unpackedLeafDat = unpackObjectCount(g_BvhNodeData[_nid].nid.w);
	uint numTriangles = unpackedLeafDat.x;
	uint numObjects = unpackedLeafDat.y;

	for(uint i=0 ; i<numObjects ; ++i)
	{
		uint objIndex = g_BvhLeafData[leafDataOffset + numTriangles + i];
		if(hitPrimitiveFast(objIndex, _ray, tmax))
			return true;
	}

	return false;
}

#if NO_BVH
void brutForceTraverse(Ray _ray, inout ClosestHit closestHit)
{
	uint matId = 0xFFFFffff;
	uint objId = 0xFFFFffff;
	for(uint i=0 ; i<g_Constants.numPrimitives ; ++i)
	{
		Hit hit;
		bool hasHit = hitPrimitive(i, _ray, closestHit.t, hit);

		closestHit.t =		hasHit ? hit.t * 0.999	: closestHit.t;
		closestHit.normal = hasHit ? hit.normal		: closestHit.normal;
		matId =				hasHit ? g_BvhPrimitiveData[i].iparam : matId;
		objId =				hasHit ? i : objId;
	}

	closestHit.nid_mid = (matId & 0xFFFF0000) >> 16;
	closestHit.objectId = objId;
}
#endif

#endif