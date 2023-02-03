#ifndef H_SYSTEM_FXH_
#define H_SYSTEM_FXH_

// Converts a color from linear light gamma to sRGB gamma
vec4 fromLinear(vec4 linearRGB)
{
    bvec4 cutoff = lessThan(linearRGB, vec4(0.0031308));
    vec4 higher = vec4(1.055)*pow(linearRGB, vec4(1.0/2.4)) - vec4(0.055);
    vec4 lower = linearRGB * vec4(12.92);

    return mix(higher, lower, cutoff);
}

// Converts a color from sRGB gamma to linear light gamma
vec4 toLinear(vec4 sRGB)
{
    bvec4 cutoff = lessThan(sRGB, vec4(0.04045));
    vec4 higher = pow((sRGB + vec4(0.055))/vec4(1.055), vec4(2.4));
    vec4 lower = sRGB/vec4(12.92);

    return mix(higher, lower, cutoff);
}

#define DYNAMIC_TEXTURE_INDEXING    1

#define USE_SHADOW					1
#define COMPUTE_SHADOW_ON_THE_FLY	1
#define NO_RAY_INVDIR				0

#define USE_SHARED_MEM		1
#define USE_TRAVERSE_TLAS   1
#define NO_LIGHTING         0

#define DEBUG_GEOMETRY		0
#define DEBUG_BVH		    0
#define DEBUG_BVH_TRAVERSAL 0
#define ANY_DEBUG           (DEBUG_GEOMETRY + DEBUG_BVH + DEBUG_BVH_TRAVERSAL)

#define SHOW_LPF_DEBUG      1
#define LPF_DEBUG_SIZE      0.1

#define g_AreaLightShadowUniformSampling	0
#define g_AreaLightShadowSampleCount	    1

#define OFFSET_RAY_COLLISION 0.9999

#define Fdielectric vec3(0.04,0.04,0.04)
#define RoughnessPBR 0.02

#endif