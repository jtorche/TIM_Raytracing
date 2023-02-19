#ifndef H_LIGHTPROBHELPERS_CPP_FXH_
#define H_LIGHTPROBHELPERS_CPP_FXH_

#include "lightprob.glsl"
#include "core/primitive_cpp.glsl"

#define LP_TILE_RES 4
#define LP_TILE_SIZE (LP_TILE_RES*LP_TILE_RES*LP_TILE_RES)

vec3 computeLpfStep(uvec3 _lpfResolution, Box _aabb)
{
	vec3 dim = _aabb.maxExtent - _aabb.minExtent;
	return dim / vec3(_lpfResolution.x, _lpfResolution.y, _lpfResolution.z);
}

uint getLightProbIndex(uvec3 _coord, uvec3 _lpfResolution)
{
#if LP_TILE_RES > 1
	uvec3 tileCoord = _coord / LP_TILE_RES;
	uvec3 tileResolution =  _lpfResolution / LP_TILE_RES;
	
	uint result = tileCoord.x + tileCoord.y * tileResolution.x + tileCoord.z * tileResolution.x * tileResolution.y;
	result *= LP_TILE_SIZE;
	
	_coord = _coord % LP_TILE_RES;
	result += _coord.x + _coord.y * LP_TILE_RES + _coord.z * LP_TILE_RES * LP_TILE_RES;
	return result;
#else
	return _coord.x + _coord.y * _lpfResolution.x + _coord.z * _lpfResolution.x * _lpfResolution.y;
#endif
}

ivec3 getLightProbCoord(uint _index, uvec3 _lpfResolution)
{
#if LP_TILE_RES > 1
	uvec3 tileResolution =  _lpfResolution / LP_TILE_RES;
	uint tileIndex = _index / LP_TILE_SIZE;
	uint inTileIndex = _index % LP_TILE_SIZE;
	
	uvec3 result;
	result.z = tileIndex / (tileResolution.x * tileResolution.y);
	tileIndex = tileIndex  % (tileResolution.x * tileResolution.y);
	result.y = tileIndex / tileResolution.x;
	result.x = tileIndex % tileResolution.x;
	
	result *= LP_TILE_RES;
	
	result.x += inTileIndex % LP_TILE_RES;
	result.y += (inTileIndex / LP_TILE_RES) % LP_TILE_RES;
	result.z += (inTileIndex / (LP_TILE_RES*LP_TILE_RES)) % LP_TILE_RES;
	
	return ivec3(result.x, result.y, result.z);
#else
	uint z = _index / (_lpfResolution.x * _lpfResolution.y);
	_index = _index  % (_lpfResolution.x * _lpfResolution.y);
	uint y = _index / _lpfResolution.x;
	uint x = _index % _lpfResolution.x;
	
	return ivec3(x,y,z);
#endif
}

vec3 getLightProbPosition(uvec3 _coord, uvec3 _lpfResolution, Box _aabb)
{
	vec3 dim = _aabb.maxExtent - _aabb.minExtent;
	vec3 step = dim / vec3(_lpfResolution.x, _lpfResolution.y, _lpfResolution.z);
	
	return _aabb.minExtent + step * vec3(_coord.x + 0.5, _coord.y + 0.5, _coord.z + 0.5);
}

vec3 getLightProbCoordUVW(vec3 _pos, uvec3 _lpfResolution, in Box _aabb, vec3 _step)
{
	vec3 uvw = ((_pos - _aabb.minExtent) / _step) - vec3(0.5,0.5,0.5); 
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