#ifndef H_LIGHTPROBHELPERS_CPP_FXH_
#define H_LIGHTPROBHELPERS_CPP_FXH_

#include "primitive_cpp.glsl"

uint getLightProbIndex(uvec3 _coord, uvec3 _lpfResolution)
{
	return _coord.x + _coord.y * _lpfResolution.x + _coord.z * _lpfResolution.x * _lpfResolution.y;
}

uvec3 getLightProbCoord(uint _index, uvec3 _lpfResolution)
{
	uint z = _index / (_lpfResolution.x * _lpfResolution.y);
	_index = _index  % (_lpfResolution.x * _lpfResolution.y);
	uint y = _index / _lpfResolution.x;
	uint x = _index % _lpfResolution.x;

	return uvec3(x,y,z);
}

vec3 getLightProbPosition(uvec3 _coord, uvec3 _lpfResolution, Box _aabb)
{
	vec3 step = _aabb.maxExtent - _aabb.minExtent;
	step /= vec3(_lpfResolution.x+2, _lpfResolution.y+2, _lpfResolution.z+2);
	return step * vec3(_coord.x+1, _coord.y+1, _coord.z+1);
}

#endif