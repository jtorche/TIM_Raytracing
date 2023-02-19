#ifndef H_LIGHTPROB_CPP_FXH_
#define H_LIGHTPROB_CPP_FXH_

#include "Shaders/core/primitive_cpp.glsl"

struct SH9
{
	float w[9];
};

struct SH9Color
{
	vec3 w[9];
};

struct LightProbFieldHeader
{
	Box aabb;
	uvec3 resolution;
	vec3 step;
};

#define NUM_RAYS_PER_PROB 64
#define UPDATE_LPF_NUM_PROBS_PER_GROUP 64
#define UPDATE_LPF_LOCALSIZE 64

struct GenLightProbFieldConstants
{
	vec4 lpfMin;
	vec4 lpfMax;
	uvec4 lpfResolution;
	vec4 sunDir;
	vec4 sunColor;
	vec4 rays[NUM_RAYS_PER_PROB];
};

#define SH_Y00		0
#define SH_Y11		1
#define SH_Y10		2
#define SH_Y1_1		3
#define SH_Y21		4
#define SH_Y2_1		5
#define SH_Y2_2		6
#define SH_Y20		7
#define SH_Y22		8

#endif