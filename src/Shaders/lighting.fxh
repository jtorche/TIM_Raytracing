#ifndef H_LIGHTING_FXH_
#define H_LIGHTING_FXH_

#include "primitive_cpp.fxh"

vec3 evalPointLight(in PointLight _pl, vec3 _pos, vec3 _normal, vec3 _eye)
{
	float d = length(_pl.pos - _pos);
	vec3 L = (_pl.pos - _pos) / d;
	float dotL = dot(_normal, L);

	float attTerm = (d / _pl.radius);
	float att = 1.0 /  ( attTerm*attTerm * 300 + 1.0 );
	//float att = mix(0,1,attTerm);

	return att * _pl.color * dotL;
}

#endif