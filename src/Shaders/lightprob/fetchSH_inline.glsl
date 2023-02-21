#ifndef H_FETCHSH_FXH_
#define H_FETCHSH_FXH_

#include "lightprob.glsl"

SH9Color fetchSH9(ivec3 _coord)
{
	SH9Color result;
	vec4 Y00 = imageLoad(g_lpfTextures[0], _coord);
	result.w[0] = Y00.xyz;

	vec4 s0R = imageLoad(g_lpfTextures[1], _coord);
	vec4 s0G = imageLoad(g_lpfTextures[3], _coord);
	vec4 s0B = imageLoad(g_lpfTextures[5], _coord);
	
	result.w[1] = vec3(s0R.x, s0G.x, s0B.x);
	result.w[2] = vec3(s0R.y, s0G.y, s0B.y);
	result.w[3] = vec3(s0R.z, s0G.z, s0B.z);
	result.w[4] = vec3(s0R.w, s0G.w, s0B.w);

	vec4 s1R = imageLoad(g_lpfTextures[2], _coord);
	vec4 s1G = imageLoad(g_lpfTextures[4], _coord);
	vec4 s1B = imageLoad(g_lpfTextures[6], _coord);

	result.w[5] = vec3(s1R.x, s1G.x, s1B.x);
	result.w[6] = vec3(s1R.y, s1G.y, s1B.y);
	result.w[7] = vec3(s1R.z, s1G.z, s1B.z);
	result.w[8] = vec3(s1R.w, s1G.w, s1B.w);

	return result;
}

SH9Color lerp(in SH9Color _sh0, in SH9Color _sh1, float _x)
{
	SH9Color result;
	for(int i=0 ; i<9 ; ++i)
		result.w[i] = mix(_sh0.w[i], _sh1.w[i], _x);
	
	return result;
}

SH9Color sh_sum(in SH9Color _sh0, in SH9Color _sh1)
{
	SH9Color result;
	for(int i=0 ; i<9 ; ++i)
		result.w[i] = _sh0.w[i] + _sh1.w[i];
	
	return result;
}

SH9Color sh_mul(in SH9Color _sh, float _x)
{
	SH9Color result;
	for(int i=0 ; i<9 ; ++i)
		result.w[i] = _sh.w[i] * _x;
	
	return result;
}

SH9Color sampleSH9(vec3 _uvw, uvec3 _resolution)
{
	ivec3 icoord = ivec3(min(int(_uvw.x), _resolution.x-2), min(int(_uvw.y), _resolution.y-2), min(int(_uvw.z), _resolution.z-2));
	vec3 fractUVW = _uvw - icoord;

	SH9Color sh00 = fetchSH9(icoord + ivec3(0,0,0));
	SH9Color sh10 = fetchSH9(icoord + ivec3(1,0,0));
	SH9Color sh01 = fetchSH9(icoord + ivec3(0,1,0));
	SH9Color sh11 = fetchSH9(icoord + ivec3(1,1,0));

	SH9Color sh_z0 = lerp(lerp(sh00, sh10, fractUVW.x), lerp(sh01, sh11, fractUVW.x), fractUVW.y);

	sh00 = fetchSH9(icoord + ivec3(0,0,1));
	sh10 = fetchSH9(icoord + ivec3(1,0,1));
	sh01 = fetchSH9(icoord + ivec3(0,1,1));
	sh11 = fetchSH9(icoord + ivec3(1,1,1));

	SH9Color sh_z1 = lerp(lerp(sh00, sh10, fractUVW.x), lerp(sh01, sh11, fractUVW.x), fractUVW.y);

	return lerp(sh_z0, sh_z1, fractUVW.z);
}

SH9Color sampleSH9_test(vec3 _uvw, uvec3 _resolution)
{
	ivec3 icoord = ivec3(min(int(_uvw.x), _resolution.x-2), min(int(_uvw.y), _resolution.y-2), min(int(_uvw.z), _resolution.z-2));
	vec3 fractUVW = _uvw - icoord;

	const float baseLength = 1.05;

	float sum = 0;
	SH9Color shResult;
	for(uint i=0 ; i<8 ; ++i)
	{
		vec3 offset = vec3(float(i & 1), float((i >> 1) & 1), float((i >> 2) & 1));
		ivec3 ioffset = ivec3(i & 1, (i >> 1) & 1, (i >> 2) & 1);

		float w = max(0, baseLength - length(fractUVW - offset));
		SH9Color sh = fetchSH9(icoord + ioffset);
		shResult = i == 0 ? sh_mul(sh, w) : sh_sum(shResult, sh_mul(sh, w));
		sum += w;
	}

	return sh_mul(shResult, 1.0 / sum);
}

SH9Color sampleSH9_test(vec3 _uvw, uvec3 _resolution, uint _lpfMask)
{
	ivec3 icoord = ivec3(min(int(_uvw.x), _resolution.x-2), min(int(_uvw.y), _resolution.y-2), min(int(_uvw.z), _resolution.z-2));
	vec3 fractUVW = _uvw - icoord;

	const float baseLength = sqrt(3);

	float sum = 0;
	SH9Color shResult;
	for(uint i=0 ; i<8 ; ++i)
	{
		vec3 offset = vec3(float(i & 1), float((i >> 1) & 1), float((i >> 2) & 1));
		ivec3 ioffset = ivec3(i & 1, (i >> 1) & 1, (i >> 2) & 1);

		float w = max(0.01, baseLength - length(fractUVW - offset));
		w = (_lpfMask & (1 << i)) == 0 ? w : 0;

		SH9Color sh = fetchSH9(icoord + ioffset);
		shResult = i == 0 ? sh_mul(sh, w) : sh_sum(shResult, sh_mul(sh, w));
		sum += w;
	}

	return sh_mul(shResult, 1.0 / sum);
}

SH9Color sampleSH9(vec3 _uvw, uvec3 _resolution, uint _lpfMask)
{
#define MASK(x,y,z) ((_lpfMask & (1 << (x + (y << 1) + (z << 2)))) == 0) 

	ivec3 icoord = ivec3(min(int(_uvw.x), _resolution.x-2), min(int(_uvw.y), _resolution.y-2), min(int(_uvw.z), _resolution.z-2));
	vec3 fractUVW = _uvw - icoord;

	SH9Color sh00 = fetchSH9(icoord + ivec3(0,0,0));
	SH9Color sh10 = fetchSH9(icoord + ivec3(1,0,0));
	SH9Color sh01 = fetchSH9(icoord + ivec3(0,1,0));
	SH9Color sh11 = fetchSH9(icoord + ivec3(1,1,0));

	SH9Color shx0 = MASK(0,0,0) && MASK(1,0,0) ? lerp(sh00, sh10, fractUVW.x) : (MASK(0,0,0) ? sh00 : sh10);
	SH9Color shx1 = MASK(0,1,0) && MASK(1,1,0) ? lerp(sh01, sh11, fractUVW.x) : (MASK(0,1,0) ? sh01 : sh11);
	
	const bool b_shx00 = MASK(0,0,0) || MASK(1,0,0);
	const bool b_shx10 = MASK(0,1,0) || MASK(1,1,0);
	SH9Color sh_z0 = b_shx00 && b_shx10 ? lerp(shx0, shx1, fractUVW.y) : (b_shx00 ? shx0 : shx1);

	sh00 = fetchSH9(icoord + ivec3(0,0,1));
	sh10 = fetchSH9(icoord + ivec3(1,0,1));
	sh01 = fetchSH9(icoord + ivec3(0,1,1));
	sh11 = fetchSH9(icoord + ivec3(1,1,1));

	shx0 = MASK(0,0,1) && MASK(1,0,1) ? lerp(sh00, sh10, fractUVW.x) : (MASK(0,0,1) ? sh00 : sh10);
	shx1 = MASK(0,1,1) && MASK(1,1,1) ? lerp(sh01, sh11, fractUVW.x) : (MASK(0,1,1) ? sh01 : sh11);
	
	const bool b_shx01 = MASK(0,0,1) || MASK(1,0,1);
	const bool b_shx11 = MASK(0,1,1) || MASK(1,1,1);
	SH9Color sh_z1 = b_shx01 && b_shx11 ? lerp(shx0, shx1, fractUVW.y) : (b_shx01 ? shx0 : shx1);

	const bool b_shxy0 = b_shx00 || b_shx10;
	const bool b_shxy1 = b_shx01 || b_shx11;
#undef MASK

	return b_shxy0 && b_shxy1 ? lerp(sh_z0, sh_z1, fractUVW.z) : (b_shxy0 ? sh_z0 : sh_z1);
}

#endif