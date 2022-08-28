#ifndef H_STRUCT_FXH_
#define H_STRUCT_FXH_

#define g_PassData_bind 0
#define g_outputImage_bind 1
#define g_BvhPrimitives_bind 2
#define g_BvhTriangles_bind 3
#define g_BvhNodes_bind 4
#define g_BlasHeaders_bind 5
#define g_BvhLeafData_bind 6
#define g_BvhLights_bind 7
#define g_BvhMaterials_bind 8
#define g_InRayBuffer_bind 9
#define g_OutReflexionRayBuffer_bind 10
#define g_OutRayBuffer_bind 11
#define g_OutRefractionRayBuffer_bind 12
#define g_inputImage_bind 13
#define g_dataTextures_bind 14

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

#define NID_LEAF_BIT 0x80000000
#define NID_MASK 0x3FFFFFFF
#define NID_LEFT_LEAF_BIT 0x80000000
#define NID_RIGHT_LEAF_BIT 0x40000000

#endif