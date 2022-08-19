#ifndef H_PRIMITIVE_FXH_
#define H_PRIMITIVE_FXH_

#define Primitive_Sphere	1
#define Primitive_AABB		2
#define Primitive_OBB		3

#define Light_Sphere		1
#define Light_Area			2

#define Material_Emissive		1
#define Material_Lambert		2
#define Material_Mirror			3
#define Material_Transparent	4
#define Material_PBR			5

// used to pack node data
#define TriangleBitCount 18
#define PrimitiveBitCount 9
#define LightBitCount 5

struct Ray
{
    vec3 from;
    vec3 dir;
	#if !NO_RAY_INVDIR
	vec3 invdir;
	#endif
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

struct Triangle
{
	uint vertexOffset;
	uint index01;
	uint index2_matId;
};

// Material_Mirror:			x:mirrorness
// Material_Transparent:	x:reflectivity, y:refraction_index	
struct Material
{
	uvec4	type_ids;
	vec4	color;
	vec4	params;
};

struct PackedPrimitive
{
	uint iparam; // type|materialId
	float fparam[6];
};

struct PackedBVHNode
{
	uvec4 nid; // 0:parent_sibling, 1:child_lr, 2:leafData_offset, 3:numPrimitive | numLights
	vec4 n0xy;
	vec4 n1xy;
	vec4 nz;
};

struct BlasHeader
{
	Box aabb;
	uint rootIndex; // Node index in PackedBVHNode list
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