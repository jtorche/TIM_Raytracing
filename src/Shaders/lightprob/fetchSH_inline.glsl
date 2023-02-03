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

#endif