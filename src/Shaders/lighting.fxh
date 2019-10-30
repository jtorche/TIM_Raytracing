#ifndef H_LIGHTING_FXH_
#define H_LIGHTING_FXH_

#include "math.fxh"
#include "primitive_cpp.fxh"

bool traverseBvhFast(Ray r, uint rootId, float tmax);

#define g_LightCapThreshold 255.0

float computeAttenuation(float _dist, float _lightRadius)
{
	float att = (_lightRadius * _lightRadius + 1) / ( g_LightCapThreshold * _dist * _dist);
	return clamp(att - (1.0 / g_LightCapThreshold), 0, 1);
}

vec3 evalPointLight(uint _rootId, in PointLight _pl, vec3 _pos, vec3 _normal, vec3 _eye)
{
	float d = length(_pl.pos - _pos);
	vec3 L = (_pl.pos - _pos) / d;
	
	float dotL = clamp(dot(_normal, L), 0, 1);
	float att = computeAttenuation(d, _pl.radius);

	if(att * dotL > 0.001)
	{
		Ray shadowRay; shadowRay.from = _pos; shadowRay.dir = L; shadowRay.invdir = vec3(1,1,1) / L;
		float shadow = traverseBvhFast(shadowRay, _rootId, TMAX) ? 0 : 1;

		return shadow * att * dotL * _pl.color;
	}

	return vec3(0,0,0);
}

vec3 evalSphereLight(uint _rootId, in SphereLight _pl, vec3 _pos, vec3 _normal, vec3 _eye)
{
	float d = length(_pl.pos - _pos);
	vec3 L = (_pl.pos - _pos) / d;
	
	float dotL = clamp(dot(_normal, L), 0, 1);
	float att = computeAttenuation(d, _pl.radius);

	if(att * dotL > 0.001)
	{
		Ray shadowRay; shadowRay.from = _pos; shadowRay.dir = L; shadowRay.invdir = vec3(1,1,1) / L;
		float shadow = traverseBvhFast(shadowRay, _rootId, d - _pl.sphereRadius) ? 0 : 1;

		return shadow * att * dotL * _pl.color;
	}

	return vec3(0,0,0);
}

vec3 evalAreaLight(uint _rootId, in AreaLight _al, vec3 _pos, vec3 _normal, vec3 _eye)
{
	vec3 areaNormal = normalize(cross(_al.width, _al.height));
	float res = 0;

#if g_AreaLightShadowUniformSampling
	#if g_AreaLightShadowSampleCount == 1
	vec3 stepX = _al.width * 0.5;
	vec3 stepY = _al.height * 0.5;
	#else
	vec3 stepX = _al.width / (g_AreaLightShadowSampleCount - 1);
	vec3 stepY = _al.height / (g_AreaLightShadowSampleCount - 1);
	#endif
#endif
	for(int i=0 ; i<g_AreaLightShadowSampleCount ; ++i)
	{
		for(int j=0 ; j<g_AreaLightShadowSampleCount ; ++j)
		{
		#if !g_AreaLightShadowUniformSampling
			vec3 stepX = _al.width  * rand( vec2(i+0.5,-j+0.5) * dot(vec3(23.727,-12.345,4.234), _pos) );
			vec3 stepY = _al.height * rand( vec2(j+0.5,i+0.5) * dot(vec3(2.727,-41.345,14.234), _pos) );
			vec3 sampledLight = _al.pos + stepX + stepY; 
		#else
			#if g_AreaLightShadowSampleCount == 1
			vec3 sampledLight = _al.pos + stepX + stepY; 
			#else
			vec3 sampledLight = _al.pos + stepX * i + stepY * j; 
			#endif
		#endif
			
			
			float d = length(sampledLight - _pos);
			vec3 L = (sampledLight - _pos) / d;
			
			float dotL = clamp(dot(_normal, L), 0, 1);
			float att = computeAttenuation(d, _al.attenuationRadius);
			float areaAtt = clamp(dot(areaNormal, -L), 0, 1);
			
			if(areaAtt * att * dotL > 0.001)
			{
				Ray shadowRay; shadowRay.from = _pos; shadowRay.dir = L; shadowRay.invdir = vec3(1,1,1) / L;
				float shadow = traverseBvhFast(shadowRay, _rootId, d) ? 0 : 1;
			
				res += shadow * att * dotL * areaAtt;
			}
		}
	}

	return _al.color * res / (g_AreaLightShadowSampleCount * g_AreaLightShadowSampleCount);
}

#endif