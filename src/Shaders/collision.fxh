#ifndef H_COLLISION_FXH_
#define H_COLLISION_FXH_

#include "primitive_cpp.fxh"

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

float CollideBox(Ray _ray, Box _box, float tmin, float tmax)
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

    return (tmin < tmax) ? tmin : -1;
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

struct ClosestHit
{
#if !USE_SHARED_MEM
    vec3 normal;
#endif
    float t;
	uint nid_mid;
};

struct Hit
{
    vec3 normal;
    float t;
	uint nid_mid;
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

bool HitBox(Ray r, Box box, float tMin, float tmax, out Hit outHit)
{
	float t = CollideBox(r, box, tMin, tmax).x;
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

#endif