#ifndef H_PRIMITIVE_FXH_
#define H_PRIMITIVE_FXH_

#define Primitive_Sphere	1
#define Primitive_AABB		2
#define Primitive_OBB		3

#define Light_Point			1
#define Light_Sphere		2
#define Light_Area			3

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
	uvec4 nid; // 0:parent_sibling, 1:child_lr, 2:leafData_offset, 3:numPrimitive | numLights
	vec4 n0xy;
	vec4 n1xy;
	vec4 nz;
};

struct PointLight // attenuation: 1 /  ( ((x/r) * 5)^2 + 1 )
{
	vec3 pos;
	float radius;
	vec3 color;
};

struct SphereLight
{
	vec3 pos;
	float radius;
	vec3 color;
	float sphereRadius;
};

struct AreaLight
{
	vec3 pos;
	vec3 width;
	vec3 height;
	vec3 color;
	float attenuationRadius;
};

struct PackedLight
{
	uint iparam;
	float fparam[13];
};

#endif