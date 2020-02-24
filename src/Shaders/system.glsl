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


#define USE_SHADOW					0
#define COMPUTE_SHADOW_ON_THE_FLY	1
#define NO_RAY_INVDIR				0

#define NO_BVH				0
#define TILE_FRUSTUM_CULL	0
#define USE_SHARED_MEM		0

#define g_AreaLightShadowUniformSampling	0
#define g_AreaLightShadowSampleCount	    1

#define OFFSET_RAY_COLLISION 0.9999

#endif