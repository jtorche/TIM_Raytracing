#ifndef H_BVH_COLLISION_FXH_
#define H_BVH_COLLISION_FXH_

#include "collision.fxh"

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

PointLight loadPointLight(uint lightIndex)
{
	PointLight l;
	l.pos = vec3(g_BvhLightData[lightIndex].fparam[0], g_BvhLightData[lightIndex].fparam[1], g_BvhLightData[lightIndex].fparam[2]);
	l.color = vec3(g_BvhLightData[lightIndex].fparam[3], g_BvhLightData[lightIndex].fparam[4], g_BvhLightData[lightIndex].fparam[5]);
	l.radius = g_BvhLightData[lightIndex].fparam[6];
	return l;
}

bool hitPrimitive(uint objIndex, Ray r, float tmax, out Hit hit)
{
	uint type =  g_BvhPrimitiveData[objIndex].iparam;

	bool hasHit = false;
	switch(type)
	{
		case Primitive_Sphere: 
		hasHit = HitSphere(r, loadSphere(objIndex), 0, tmax, hit);
		break;

		case Primitive_AABB: 
		hasHit = HitBox(r, loadBox(objIndex), 0, tmax, hit);
		break;
	}

	return hasHit;
}

void bvh_collide(uint _nid, Ray _ray, inout Hit closestHit)
{
	_nid = _nid & 0x7FFF;
	uint leafDataOffset = g_BvhNodeData[_nid].nid.z;
	uint numObjects = g_BvhNodeData[_nid].nid.w & 0xFFFF;

	for(uint i=0 ; i<numObjects ; ++i)
	{
		uint objIndex = g_BvhLeafData[leafDataOffset + i];
		Hit hit;
		bool hasHit = hitPrimitive(objIndex, _ray, closestHit.t, hit);

		closestHit.t =		hasHit ? hit.t * 0.999	: closestHit.t;
		closestHit.normal = hasHit ? hit.normal		: closestHit.normal;
		closestHit.nid =	hasHit ? _nid			: closestHit.nid;
	}
}

void brutForceTraverse(Ray _ray, inout Hit closestHit)
{
	for(uint i=0 ; i<g_Constants.numPrimitives ; ++i)
	{
		Hit hit;
		bool hasHit = hitPrimitive(i, _ray, closestHit.t, hit);

		closestHit.t =		hasHit ? hit.t * 0.999	: closestHit.t;
		closestHit.normal = hasHit ? hit.normal		: closestHit.normal;
	}
}

#endif