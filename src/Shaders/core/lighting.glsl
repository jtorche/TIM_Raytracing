#ifndef H_LIGHTING_FXH_
#define H_LIGHTING_FXH_

#include "math.glsl"
#include "primitive_cpp.glsl"

#define PI 3.141592
#define Epsilon 0.00001

bool traverseBvhFast(Ray r, uint rootId, float tmax);
bool traverseTlasFast(Ray r, uint rootId, float tmax);
vec3 computePbrLighting(vec3 albedo, float metalness, vec3 lightColor, vec3 v, vec3 l, vec3 n);

#define g_LightCapThreshold 255.0

float computeAttenuation(float _dist, float _lightRadius)
{
	float att = (_lightRadius * _lightRadius + 1) / (g_LightCapThreshold * _dist * _dist);
	return clamp(att - (1.0 / g_LightCapThreshold), 0, 1);
}

vec3 computeLighting(uint rootId, in Material _mat, vec3 _texColor, vec3 _lightColor, vec3 P, vec3 L, vec3 N, vec3 E, float att, float shadowRayLength)
{
	if(_mat.type_ids.x == Material_Emissive)
		return _mat.color.xyz;

	if(_mat.type_ids.x == Material_Transparent)
	    return vec3(0,0,0);
	
	vec3 lit = vec3(0, 0, 0);
	float dotL = clamp(dot(N, L), 0, 1);

	if(_mat.type_ids.x == Material_PBR)
		lit = computePbrLighting(_mat.color.xyz * _texColor, _mat.params.x, _lightColor, normalize(E-P), L, N);
	else if(_mat.type_ids.x == Material_Lambert)
		lit = dotL * _lightColor * _mat.color.xyz * _texColor;

	lit *= att;

	#if USE_SHADOW && COMPUTE_SHADOW_ON_THE_FLY
	if(att * dotL > 0.001)
	{
		Ray shadowRay; shadowRay.from = P; shadowRay.dir = L; 
		#if !NO_RAY_INVDIR   
		shadowRay.invdir = vec3(1,1,1) / L;
		#endif
	#ifdef USE_TRAVERSE_TLAS
		float shadow = traverseTlasFast(shadowRay, rootId, shadowRayLength) ? 0 : 1;
	#else
		float shadow = traverseBvhFast(shadowRay, rootId, shadowRayLength) ? 0 : 1;
	#endif
		lit *= shadow;
	}
	#endif

	return lit;
}

vec3 computeIndirectLighting(in Material _mat, vec3 _albedo, vec3 _light)
{
	if(_mat.type_ids.x == Material_Emissive || _mat.type_ids.x == Material_Transparent)
		return vec3(0,0,0);

	if(_mat.type_ids.x == Material_PBR)
	{
		float metalness = _mat.params.x;
		return (1.0 - metalness) * _albedo * _mat.color.xyz * _light / TIM_PI;
	}
	else if(_mat.type_ids.x == Material_Lambert)
		return _light * _albedo * _mat.color.xyz / TIM_PI;

	return vec3(0,0,0);
}

vec3 evalSphereLight(uint _rootId, in SphereLight _sl, in Material _mat, in vec3 _texColor, vec3 _pos, vec3 _normal, vec3 _eye)
{
	float d = length(_sl.pos - _pos);
	if(d < _sl.radius)
	{
		vec3 L = (_sl.pos - _pos) / d;
		d = max(0.01, d - _sl.sphereRadius);

		return computeLighting(_rootId, _mat, _texColor, _sl.color, _pos, L, _normal, _eye, computeAttenuation(d, _sl.radius), d);
	}
}

vec3 evalSunLight(uint _rootId, in vec3 _sunDir, in vec3 _sunColor, in Material _mat, in vec3 _texColor, vec3 _pos, vec3 _normal, vec3 _eye)
{
	return computeLighting(_rootId, _mat, _texColor, _sunColor, _pos, -_sunDir, _normal, _eye, 1, 1000);
}

vec3 evalAreaLight(uint _rootId, in AreaLight _al, in Material _mat, in vec3 _texColor, vec3 _pos, vec3 _normal, vec3 _eye)
{
	vec3 areaNormal = normalize(cross(_al.width, _al.height));
	vec3 res = vec3(0,0,0);

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

			float areaAtt = clamp(dot(areaNormal, -L), 0, 1);
			float att = computeAttenuation(d, _al.attenuationRadius);

			res += computeLighting(_rootId, _mat, _texColor, _al.color, _pos, L, _normal, _eye, att * areaAtt, d);
		}
	}

	return  res / (g_AreaLightShadowSampleCount * g_AreaLightShadowSampleCount);
}

// PBR equation
// https://google.github.io/filament/Filament.html#toc4.8
float D_GGX(float NoH, float a) 
{
	float a2 = a * a;
	float f = (NoH * a2 - NoH) * NoH + 1.0;
	return a2 / (PI * f * f);
}

vec3 F_Schlick(float u, vec3 f0) 
{
	return f0 + (vec3(1.0) - f0) * pow(1.0 - u, 5.0);
}

float V_SmithGGXCorrelated(float NoV, float NoL, float a) 
{
	float a2 = a * a;
	float GGXL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
	float GGXV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
	return 0.5 / (GGXV + GGXL);
}

vec3 computePbrLighting(vec3 albedo, float metalness, vec3 lightColor, vec3 v, vec3 l, vec3 n)
{
	vec3 h = normalize(v + l);

	float NoV = abs(dot(n, v)) + 1e-5;
	float NoL = clamp(dot(n, l), 0.0, 1.0);
	float NoH = clamp(dot(n, h), 0.0, 1.0);
	float LoH = clamp(dot(l, h), 0.0, 1.0);

	// perceptually linear roughness to roughness (see parameterization)
	float roughness = RoughnessPBR * RoughnessPBR;

	float D = D_GGX(NoH, roughness);
	vec3 f0 = mix(Fdielectric, albedo, metalness);
	vec3  F = F_Schlick(LoH, f0);
	float V = V_SmithGGXCorrelated(NoV, NoL, roughness);

	// specular BRDF
	vec3 Fr = (D * V) * F;

	// diffuse BRDF
	vec3 diffuseColor = (1.0 - metalness) * albedo;
	vec3 Fd = diffuseColor / PI;

	return (Fd + Fr) * lightColor * NoL;
}

#endif