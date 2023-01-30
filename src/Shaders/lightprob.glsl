#ifndef H_LIGHTPROB_CPP_FXH_
#define H_LIGHTPROB_CPP_FXH_

#include "primitive_cpp.glsl"

struct SH9
{
	float w[9];
};

struct SH9Color
{
	SH9 r;
	SH9 g;
	SH9 b;
};

#define NUM_RAYS_PER_PROB 64
#define UPDATE_LPF_NUM_PROBS_PER_GROUP 256

struct GenLightProbFieldConstants
{
	vec4 lpfMin;
	vec4 lpfMax;
	uvec4 lpfResolution;
	vec4 sunDir;
	vec4 sunColor;
	vec4 rays[NUM_RAYS_PER_PROB];
};

#endif