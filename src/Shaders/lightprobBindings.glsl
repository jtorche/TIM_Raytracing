#ifndef H_LIGHTPROBBINDINGS_FXH_
#define H_LIGHTPROBBINDINGS_FXH_

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

struct LightProbFieldHeader
{
	Box aabb;
	uint3 resolution;
};

#endif