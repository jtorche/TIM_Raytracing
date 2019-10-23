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
    float t1 = (_box.minExtent.x - rpos.x) * r.invdir.x;
    float t2 = (_box.maxExtent.x - rpos.x) * r.invdir.x;
    float t3 = (_box.minExtent.y - rpos.y) * r.invdir.y;
    float t4 = (_box.maxExtent.y - rpos.y) * r.invdir.y;
    float t5 = (_box.minExtent.z - rpos.z) * r.invdir.z;
    float t6 = (_box.maxExtent.z - rpos.z) * r.invdir.z;

    float t7 = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
    float t8 = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

    float t9 = (t8 < 0 || t7 > t8) ? -1 : t7;

    return t9;
}

struct Hit
{
    vec3 pos;
    vec3 normal;
    float t;
};

bool HitSphere(Ray r, Sphere s, float tMin, float tMax, out Hit outHit)
{
    vec3 oc = r.from - s.center;
    float b = dot(oc, r.dir);
    float c = dot(oc, oc) - s.radius*s.radius;
    float discr = b * b - c;
    if (discr > 0)
    {
        float discrSq = sqrt(discr);

        float t = (-b - discrSq);
        if (t < tMax && t > tMin)
        {
            outHit.pos = r.from + r.dir * t;
            outHit.normal = (outHit.pos - s.center) * s.invRadius;
            outHit.t = t;
            return true;
        }
        t = (-b + discrSq);
        if (t < tMax && t > tMin)
        {
            outHit.pos = r.from + r.dir * t;
            outHit.normal = (outHit.pos - s.center) * s.invRadius;
            outHit.t = t;
            return true;
        }
    }
    return false;
}

#endif