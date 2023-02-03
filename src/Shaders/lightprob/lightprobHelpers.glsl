#ifndef H_LIGHTPROBHELPERS_CPP_FXH_
#define H_LIGHTPROBHELPERS_CPP_FXH_

#include "lightprob.glsl"
#include "core/primitive_cpp.glsl"

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
	vec3 dim = _aabb.maxExtent - _aabb.minExtent;
	vec3 step = dim / vec3(_lpfResolution.x, _lpfResolution.y, _lpfResolution.z);
	
	return _aabb.minExtent + step * vec3(_coord.x + 0.5, _coord.y + 0.5, _coord.z + 0.5);
}

vec3 getLightProbCoordUVW(vec3 _pos, uvec3 _lpfResolution, Box _aabb)
{
	vec3 dim = _aabb.maxExtent - _aabb.minExtent;
	vec3 step = dim / vec3(_lpfResolution.x, _lpfResolution.y, _lpfResolution.z);
	
	vec3 uvw = ((_pos - _aabb.minExtent) / step) - vec3(0.5,0.5,0.5); 
	uvw = min(max(uvw, vec3(0,0,0)), vec3(_lpfResolution.x - 1.0, _lpfResolution.y - 1.0, _lpfResolution.z - 1.0));

	return uvw;
}

SH9Color getZeroInitializedSHCoef()
{
	SH9Color coef;
	for(uint i=0 ; i<9 ; ++i)
		coef.w[i] = vec3(0,0,0);

	return coef;
}

vec3 evalSH(in SH9Color _sh, vec3 _n)
{
	float c1 = 0.429043; 
	float c2 = 0.511664;
	float c3 = 0.743125;
	float c4 = 0.886227; 
	float c5 = 0.247708;

	return
	c1 * (_n.x*_n.x - _n.y*_n.y) * _sh.w[SH_Y22] + 
	c3 * _sh.w[SH_Y20] * _n.z*_n.z + 
	c4 * _sh.w[SH_Y00] - c5 * _sh.w[SH_Y20] + 
	2 * c1 * (_sh.w[SH_Y2_2] * _n.x * _n.y + _sh.w[SH_Y21] * _n.x * _n.z + _sh.w[SH_Y2_1] * _n.y * _n.z) +
	2 * c2 * (_sh.w[SH_Y11] * _n.x + _sh.w[SH_Y1_1] * _n.y + _sh.w[SH_Y10] * _n.z);
}

#endif