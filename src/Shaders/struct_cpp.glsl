#ifndef H_STRUCT_FXH_
#define H_STRUCT_FXH_

#define LOCAL_SIZE 16
#define TMAX 100
#define TEXTURE_ARRAY_SIZE 64

#define g_TextureBrdf 0

struct PassData
{
	uvec2 frameSize;
	vec2 invFrameSize;
	vec4 cameraPos;
	vec4 cameraDir;
	
	vec4 frustumCorner00;
	vec4 frustumCorner10;
	vec4 frustumCorner01;
	mat4 invProjView;

	vec4 sunDir;
	vec4 sunColor;

	vec4 sceneMinExtent;
	vec4 sceneMaxExtent;
	uvec4 lpfResolution;
};

struct PushConstants
{
	uint numTriangles;
	uint numBlas;
	uint numPrimitives;
	uint numLights;
	uint numNodes;
};

struct IndirectLightRay
{
	vec4 pos;
	vec4 dir;
	vec4 lit;
};

struct SunDirColor
{
	vec3 sunDir;
	vec3 sunColor;
};

#define NID_LEAF_BIT 0x80000000
#define NID_MASK 0x3FFFFFFF
#define NID_LEFT_LEAF_BIT 0x80000000
#define NID_RIGHT_LEAF_BIT 0x40000000

#endif