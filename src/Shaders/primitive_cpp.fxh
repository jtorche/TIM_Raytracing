#ifndef H_PRIMITIVE_FXH_
#define H_PRIMITIVE_FXH_

#define Primitive_Sphere	1
#define Primitive_AABB		2
#define Primitive_OBB		3

struct Ray
{
    vec3 from;
    vec3 dir;
	vec3 invdir;
};

struct Box
{
	vec3 minExtent;
	vec3 maxExtent;
};

struct Sphere
{
    vec3 center;
    float radius;
    float invRadius;
};

struct Material
{
	vec3 color;
	float emissive;
};

struct PackedPrimitive
{
	uint iparam;
	float fparam[6];
};

struct PackedBVHNode
{
	float minExtent[3];
	float maxExtent[3];
	uint child_lr;
	uint objectOffset;
	uint numObjects_parent;
};

#endif