#ifndef H_LIGHTING_FXH_
#define H_LIGHTING_FXH_

#include "math.glsl"
#include "primitive_cpp.glsl"

#define PI 3.141592
#define Epsilon 0.00001

bool traverseBvhFast(Ray r, uint rootId, float tmax);
vec3 computePbrLighting(vec3 albedo,float metalness, vec3 lightColor, vec3 P, vec3 L, vec3 N, vec3 E);

#define g_LightCapThreshold 255.0

float computeAttenuation(float _dist, float _lightRadius)
{
	float att = (_lightRadius * _lightRadius + 1) / ( g_LightCapThreshold * _dist * _dist);
	return clamp(att - (1.0 / g_LightCapThreshold), 0, 1);
}

vec3 computeLighting(uint rootId, in Material _mat, vec3 lightColor, vec3 P, vec3 L, vec3 N, vec3 E, float att, float shadowRayLength)
{
	if(_mat.type_ids.x == Material_Emissive)
		return _mat.color.xyz;

	if(_mat.type_ids.x == Material_Transparent)
	    return vec3(0,0,0);
	
	if(_mat.type_ids.x == Material_PBR)
	{
		return computePbrLighting(_mat.color.xyz, _mat.params.x, lightColor, P, L, N, E);
	}

	float dotL = clamp(dot(N, L), 0, 1);

	if(_mat.type_ids.x == Material_Mirror)
		dotL = (1.0 - _mat.params.x) * dotL;

	vec3 lit = vec3(0,0,0);
	#if USE_SHADOW && COMPUTE_SHADOW_ON_THE_FLY
	if(att * dotL > 0.001)
	{
		Ray shadowRay; shadowRay.from = P; shadowRay.dir = L; 
		#if !NO_RAY_INVDIR   
		shadowRay.invdir = vec3(1,1,1) / L;
		#endif
		float shadow = traverseBvhFast(shadowRay, rootId, shadowRayLength) ? 0 : 1;
	
		lit = shadow * att * dotL * lightColor * _mat.color.xyz;
	}
	#else
	lit =  att * dotL * lightColor * _mat.color.xyz;
	#endif

	return lit;
}

vec3 evalSphereLight(uint _rootId, in SphereLight _sl, in Material _mat, vec3 _pos, vec3 _normal, vec3 _eye)
{
	float d = length(_sl.pos - _pos);
	vec3 L = (_sl.pos - _pos) / d;
	d = max(0.01, d - _sl.sphereRadius);

	return computeLighting(_rootId, _mat, _sl.color, _pos, L, _normal, _eye, computeAttenuation(d, _sl.radius), d);
}

vec3 evalAreaLight(uint _rootId, in AreaLight _al, in Material _mat, vec3 _pos, vec3 _normal, vec3 _eye)
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

			res += computeLighting(_rootId, _mat, _al.color, _pos, L, _normal, _eye, att * areaAtt, d);
		}
	}

	return  res / (g_AreaLightShadowSampleCount * g_AreaLightShadowSampleCount);
}

// PBR equation
// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float ndfGGX(float cosLh, float roughness)
{
	float alpha   = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

// Single term for separable Schlick-GGX below.
float gaSchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float gaSchlickGGX(float cosLi, float cosLo, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
	return gaSchlickG1(cosLi, k) * gaSchlickG1(cosLo, k);
}

// Shlick's approximation of the Fresnel factor.
vec3 fresnelSchlick(vec3 F0, float cosTheta)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 computePbrLighting(vec3 albedo, float metalness, vec3 lightColor, vec3 P, vec3 L, vec3 N, vec3 E)
{
	vec3 Lo = normalize(E - P);
	float cosLo = max(0.0, dot(N, Lo));
	vec3 Lr = 2.0 * cosLo * N - Lo;
	
	// Fresnel reflectance at normal incidence (for metals use albedo color).
	float roughness = RoughnessPBR;
	vec3 F0 = mix(Fdielectric, albedo, metalness);
	
	// Half-vector between Li and Lo.
	vec3 Lh = normalize(L + Lo);
	
	// Calculate angles between surface normal and various light vectors.
	float cosLi = max(0.0, dot(N, L));
	float cosLh = max(0.0, dot(N, Lh));
	
	// Calculate Fresnel term for direct lighting. 
	vec3 F  = fresnelSchlick(F0, max(0.0, dot(Lh, Lo)));
	// Calculate normal distribution for specular BRDF.
	float D = ndfGGX(cosLh, roughness);
	// Calculate geometric attenuation for specular BRDF.
	float G = gaSchlickGGX(cosLi, cosLo, roughness);
	
	// Diffuse scattering happens due to light being refracted multiple times by a dielectric medium.
	// Metals on the other hand either reflect or absorb energy, so diffuse contribution is always zero.
	// To be energy conserving we must scale diffuse BRDF contribution based on Fresnel factor & metalness.
	vec3 kd = mix(vec3(1, 1, 1) - F, vec3(0, 0, 0), metalness);
	
	// Lambert diffuse BRDF.
	// We don't scale by 1/PI for lighting & material units to be more convenient.
	// See: https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
	vec3 diffuseBRDF = kd * albedo;
	
	// Cook-Torrance specular microfacet BRDF.
	vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * cosLo);
	
	// Total contribution for this light.
	return (diffuseBRDF + specularBRDF) * lightColor * cosLi;
}

#endif