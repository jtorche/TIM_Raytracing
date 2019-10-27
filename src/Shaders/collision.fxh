#ifndef H_COLLISION_FXH_
#define H_COLLISION_FXH_

#include "primitive_cpp.fxh"

Ray createRay(vec3 _from, vec3 _dir)
{
	Ray r;
	r.from = _from;
	r.dir = _dir;
	r.invdir = vec3(1,1,1) / _dir;
	return r;
}

float CollideBox(Ray r, Box _box)
{
    float t1 = (_box.minExtent.x - r.from.x) * r.invdir.x;
    float t2 = (_box.maxExtent.x - r.from.x) * r.invdir.x;
    float t3 = (_box.minExtent.y - r.from.y) * r.invdir.y;
    float t4 = (_box.maxExtent.y - r.from.y) * r.invdir.y;
    float t5 = (_box.minExtent.z - r.from.z) * r.invdir.z;
    float t6 = (_box.maxExtent.z - r.from.z) * r.invdir.z;

    float t7 = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
    float t8 = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

    float t9 = (t8 < 0 || t7 > t8) ? -1 : t7;

    return t9;
}

struct Hit
{
    vec3 normal;
    float t;
};

bool HitSphere(Ray r, Sphere s, float tMin, out Hit outHit)
{
    vec3 oc = r.from - s.center;
    float b = dot(oc, r.dir);
    float c = dot(oc, oc) - s.radius*s.radius;
    float discr = b * b - c;
    if (discr > 0)
    {
        float discrSq = sqrt(discr);

        float t = (-b - discrSq);
        if (t > tMin)
        {
			vec3 pos = r.from + t * r.dir;
            outHit.normal = (pos - s.center) * s.invRadius;
            outHit.t = t;
            return true;
        }
        t = (-b + discrSq);
        if (t > tMin)
        {
			vec3 pos = r.from + t * r.dir;
            outHit.normal = (pos - s.center) * s.invRadius;
            outHit.t = t;
            return true;
        }
    }
    return false;
}

bool HitPlane(Ray r, vec4 _plane, float tMin, inout Hit _outHit)
{
    float denom = dot(-_plane.xyz, r.dir); 
    if (denom > 1e-6) 
	{ 
        vec3 p0l0 = (-_plane.xyz * _plane.w) - r.from; 
        float t = dot(p0l0, -_plane.xyz) / denom; 

		if(t >= tMin)
		{
			_outHit.t = t;
			_outHit.normal = _plane.xyz;
			return true; 
		}
    } 
 
    return false; 
}

#endif