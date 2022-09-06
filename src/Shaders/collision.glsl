#ifndef H_COLLISION_FXH_
#define H_COLLISION_FXH_

#include "primitive_cpp.glsl"

Ray createRay(vec3 _from, vec3 _dir)
{
	Ray r;
	r.from = _from;
	r.dir = _dir;
	#if !NO_RAY_INVDIR   
	r.invdir = vec3(1,1,1) / _dir;
	#endif
	return r;
}

bool isPointInBox(in Box _box, vec3 _point)
{
    return _point.x > _box.minExtent.x && _point.y > _box.minExtent.y && _point.z > _box.minExtent.z &&
           _point.x < _box.maxExtent.x && _point.y < _box.maxExtent.y && _point.z < _box.maxExtent.z;
}

float CollideBox(Ray _ray, Box _box, float tmin, float tmax, bool returnTmin)
{
#if !NO_RAY_INVDIR
    vec3 t0s = (_box.minExtent - _ray.from) * _ray.invdir;
    vec3 t1s = (_box.maxExtent - _ray.from) * _ray.invdir;
#else
    vec3 t0s = (_box.minExtent - _ray.from) / _ray.dir;
    vec3 t1s = (_box.maxExtent - _ray.from) / _ray.dir;
#endif

    vec3 tsmaller = min(t0s, t1s);
    vec3 tbigger = max(t0s, t1s);

    tmin = max(tmin, max(tsmaller[0], max(tsmaller[1], tsmaller[2])));
    tmax = min(tmax, min(tbigger[0], min(tbigger[1], tbigger[2])));

    return (tmin < tmax) ? (returnTmin ? tmin : tmax) : -1;
}

bool CollideSphere(Ray r, Sphere s, float tmin, float tmax)
{
	vec3 oc = r.from - s.center;
    float b = dot(oc, r.dir);
    float c = dot(oc, oc) - s.radius*s.radius;
    float discr = b * b - c;
    if (discr > 0)
    {
        float discrSq = sqrt(discr);
        float t0 = (-b - discrSq);
		float t1 = (-b + discrSq);
		return (tmin < t0 && t0 < tmax) || (tmin < t1 && t1 < tmax);  
    }
    return false;
}

float CollideTriangle(Ray r, vec3 p0, vec3 p1, vec3 p2, float tmax)
{
	// https://fr.wikipedia.org/wiki/Algorithme_d%27intersection_de_M%C3%B6ller%E2%80%93Trumbore

    const float EPSILON = 0.00001;
    vec3 edge1 = p1 - p0;
    vec3 edge2 = p2 - p0;
    vec3 h = cross(r.dir, edge2);
    float a = dot(edge1, h);
    if (abs(a) < EPSILON)
        return -1;

    float f = 1.0 / a;
    vec3 s = r.from - p0;
    float u = f * dot(s, h);
    if (clamp(u, 0.0, 1.0) != u)
        return -1;

    vec3 q = cross(s, edge1);
    float v = f * dot(r.dir, q);
    if (v < 0.0 || u + v > 1.0)
        return -1;

    float t = f * dot(edge2, q);
    if (t > EPSILON && t < tmax)
    {
        return t;
    }
    else // On a bien une intersection de droite, mais pas de rayon.
        return -1;
}

struct ClosestHit
{
#if !USE_SHARED_MEM
    vec3 texColor;
    vec3 normal;
	vec2 uv;
#endif
    float t;
	uint nid;
	uint mid_objId;
#if DEBUG_GEOMETRY
    uint dbgColorId;
#endif
};

#if DEBUG_GEOMETRY
void ClosestHit_setDebugColorId(inout ClosestHit hit, uint id) { hit.dbgColorId = id; }
#else
void ClosestHit_setDebugColorId(inout ClosestHit hit, uint id) { }
#endif

uint getObjectId(in ClosestHit _hit)
{
	return (_hit.mid_objId & 0x0000FFFF);
}

uint getMaterialId(in ClosestHit _hit)
{
	return (_hit.mid_objId & 0xFFFF0000) >> 16;
}

struct Hit
{
    vec3 normal;
    float t;
	uint nid_mid;
	vec2 uv;
};

bool HitSphere(Ray r, Sphere s, float tMin, float tmax, out Hit outHit)
{
    vec3 oc = r.from - s.center;
    float b = dot(oc, r.dir);
    float c = dot(oc, oc) - s.radius*s.radius;
    float discr = b * b - c;
    if (discr > 0)
    {
        float discrSq = sqrt(discr);

        float t = (-b - discrSq);
        if (t > tMin && t < tmax)
        {
			vec3 pos = r.from + t * r.dir;
            outHit.normal = (pos - s.center) * s.invRadius;
            outHit.t = t;
            return true;
        }
        t = (-b + discrSq);
        if (t > tMin && t < tmax)
        {
			vec3 pos = r.from + t * r.dir;
            outHit.normal = (pos - s.center) * s.invRadius;
            outHit.t = t;
            return true;
        }
    }
    return false;
}

bool HitSphereThrough(Ray r, Sphere s, float tMin, float tmax, out Hit outHit)
{
    vec3 oc = r.from - s.center;
    float b = dot(oc, r.dir);
    float c = dot(oc, oc) - s.radius*s.radius;
    float discr = b * b - c;
    if (discr > 0)
    {
        float discrSq = sqrt(discr);

		float t = (-b + discrSq);
        if (t > tMin && t < tmax)
        {
			vec3 pos = r.from + t * r.dir;
            outHit.normal = (pos - s.center) * s.invRadius;
            outHit.t = t;
            return true;
        }
        t = (-b - discrSq);
        if (t > tMin && t < tmax)
        {
			vec3 pos = r.from + t * r.dir;
            outHit.normal = (pos - s.center) * s.invRadius;
            outHit.t = t;
            return true;
        }
    }
    return false;
}

bool HitBox(Ray r, Box box, float tMin, float tmax, out Hit outHit)
{
	float t = CollideBox(r, box, tMin, tmax, true).x;
	if(t >= tMin)
	{
		vec3 pos = r.from + r.dir * t;
		vec3 center = (box.maxExtent + box.minExtent) * 0.5;

		vec3 p = pos - center;
		vec3 d = (box.maxExtent - box.minExtent) * 0.5;

		vec3 normal = 1.0001 * (p / d);

		outHit.t = t;
		outHit.normal = vec3(int(normal.x), int(normal.y), int(normal.z));

		return true;
	}

	return false;
}

bool HitBoxThrough(Ray r, Box box, float tMin, float tmax, out Hit outHit)
{
	float t = CollideBox(r, box, tMin, tmax, false).x;
	if(t >= tMin)
	{
		vec3 pos = r.from + r.dir * t;
		vec3 center = (box.maxExtent + box.minExtent) * 0.5;

		vec3 p = pos - center;
		vec3 d = (box.maxExtent - box.minExtent) * 0.5;

		vec3 normal = 1.0001 * (p / d);

		outHit.t = t;
		outHit.normal = vec3(int(normal.x), int(normal.y), int(normal.z));

		return true;
	}

	return false;
}

bool sphereFrustum4Collision(in Sphere _sphere, in vec4 _plans[4])
{
	for(uint i=0 ; i<4 ; ++i)
	{
		if(dot(_plans[i].xyz, _sphere.center) + _plans[i].w < -_sphere.radius)
			return false;
	}
	return true;
}

bool boxFrustum4Collision(in Box _box, in vec4 _plans[4])
{
	for(uint i=0 ; i<4 ; ++i)
	{
		vec3 boxClose = vec3( 
			_plans[i].x < 0 ? _box.minExtent.x : _box.maxExtent.x, 
			_plans[i].y < 0 ? _box.minExtent.y : _box.maxExtent.y,
			_plans[i].z < 0 ? _box.minExtent.z : _box.maxExtent.z );

		float d = dot(boxClose, _plans[i].xyz) + _plans[i].w;
		if(d < 0)
			return false;
	}
	return true;
}

#endif