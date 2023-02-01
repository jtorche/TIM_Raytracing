#ifndef H_LIGHTPROBHELPERS_CPP_FXH_
#define H_LIGHTPROBHELPERS_CPP_FXH_

#include "primitive_cpp.glsl"

uint getLightProbIndex(uvec3 _coord, uvec3 _lpfResolution)
{
	return _coord.x + _coord.y * _lpfResolution.x + _coord.z * _lpfResolution.x * _lpfResolution.y;
}

ivec3 getLightProbCoord(uint _index, uvec3 _lpfResolution)
{
	uint z = _index / (_lpfResolution.x * _lpfResolution.y);
	_index = _index  % (_lpfResolution.x * _lpfResolution.y);
	uint y = _index / _lpfResolution.x;
	uint x = _index % _lpfResolution.x;

	return ivec3(x,y,z);
}

vec3 getLightProbPosition(uvec3 _coord, uvec3 _lpfResolution, Box _aabb)
{
	vec3 step = _aabb.maxExtent - _aabb.minExtent;
	step /= vec3(_lpfResolution.x+2, _lpfResolution.y+2, _lpfResolution.z+2);
	return _aabb.minExtent + step * vec3(_coord.x+1, _coord.y+1, _coord.z+1);
}

vec3 getLightProbCoordUVW(vec3 _pos, uvec3 _lpfResolution, Box _aabb)
{
	vec3 step = _aabb.maxExtent - _aabb.minExtent;
	vec3 uvw = (_pos - _aabb.minExtent) / step;
	uvw *= vec3(_lpfResolution.x+2, _lpfResolution.y+2, _lpfResolution.z+2);

	uvw = min(uvw - vec3(1,1,1), vec3(0,0,0));
	uvw = min(uvw, vec3(_lpfResolution.x, _lpfResolution.y, _lpfResolution.z));

	return uvw;
}

SH9Color getZeroInitializedSHCoef()
{
	SH9Color coef;
	for(uint i=0 ; i<9 ; ++i)
		coef.w[i] = vec3(0,0,0);

	return coef;
}

vec3 getIrradianceFromSH(SH9Color shCoefs, vec3 N)
{
	return vec3(0,0,0);
}

SH9Color fetchSH9(ivec3 _coord, in sampler3D _shTex[7])
{
	SH9Color result;
	vec4 Y00 = texelFetch(_shTex[0], _coord, 0);
	result.w[0] = Y00.xyz;

	vec4 s0R = texelFetch(_shTex[1], _coord, 0);
	vec4 s0G = texelFetch(_shTex[3], _coord, 0);
	vec4 s0B = texelFetch(_shTex[5], _coord, 0);
	
	result.w[1] = vec3(s0R.x, s0G.x, s0B.x);
	result.w[2] = vec3(s0R.y, s0G.y, s0B.y);
	result.w[3] = vec3(s0R.z, s0G.z, s0B.z);
	result.w[4] = vec3(s0R.w, s0G.w, s0B.w);

	vec4 s1R = texelFetch(_shTex[2], _coord, 0);
	vec4 s1G = texelFetch(_shTex[4], _coord, 0);
	vec4 s1B = texelFetch(_shTex[6], _coord, 0);

	result.w[5] = vec3(s1R.x, s1G.x, s1B.x);
	result.w[6] = vec3(s1R.y, s1G.y, s1B.y);
	result.w[7] = vec3(s1R.z, s1G.z, s1B.z);
	result.w[8] = vec3(s1R.w, s1G.w, s1B.w);

	return result;
}

SH9Color sampleSH9(vec3 _uvw, in sampler3D _shTex[7])
{
	SH9Color result;
	vec4 Y00 = textureLod(_shTex[0], _uvw, 0);
	result.w[0] = Y00.xyz;

	vec4 s0R = textureLod(_shTex[1], _uvw, 0);
	vec4 s0G = textureLod(_shTex[3], _uvw, 0);
	vec4 s0B = textureLod(_shTex[5], _uvw, 0);
	
	result.w[1] = vec3(s0R.x, s0G.x, s0B.x);
	result.w[2] = vec3(s0R.y, s0G.y, s0B.y);
	result.w[3] = vec3(s0R.z, s0G.z, s0B.z);
	result.w[4] = vec3(s0R.w, s0G.w, s0B.w);

	vec4 s1R = textureLod(_shTex[2], _uvw, 0);
	vec4 s1G = textureLod(_shTex[4], _uvw, 0);
	vec4 s1B = textureLod(_shTex[6], _uvw, 0);

	result.w[5] = vec3(s1R.x, s1G.x, s1B.x);
	result.w[6] = vec3(s1R.y, s1G.y, s1B.y);
	result.w[7] = vec3(s1R.z, s1G.z, s1B.z);
	result.w[8] = vec3(s1R.w, s1G.w, s1B.w);

	return result;
}

#endif