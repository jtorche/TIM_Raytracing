#ifndef H_STRUCT_FXH_
#define H_STRUCT_FXH_

#define g_PassData_bind 0
#define g_outputImage_bind 1
#define g_BvhPrimitives_bind 2
#define g_BvhNodes_bind 3
#define g_BvhLeafData_bind 4
#define g_BvhLights_bind 5
#define g_BvhMaterials_bind 6

#define LOCAL_SIZE 16
#define TMAX 100

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
};

struct PushConstants
{
	uint numPrimitives;
	uint numLights;
	uint numNodes;
};

#endif