#ifndef H_RAYSTORAGEHELPERS_FXH_
#define H_RAYSTORAGEHELPERS_FXH_

#include "struct_cpp.glsl"

#if defined(FIRST_RECURSION_STEP)
layout(std430, binding = g_OutReflexionRayBuffer_bind) writeonly buffer g_OutReflexionRayBuffer_bind_layout
{
    IndirectLightRay g_outReflexionRays[];
};
layout(std430, binding = g_OutRefractionRayBuffer_bind) writeonly buffer g_OutRefractionRayBuffer_bind_layout
{
    IndirectLightRay g_outRefractionRays[];
};

#elif defined(CONTINUE_RECURSION)
layout(std430, binding = g_OutRayBuffer_bind) writeonly buffer g_OutRayBuffer_bind_layout
{
    IndirectLightRay g_outRays[];
};
#endif

uint rayIndexFromCoord()
{
	return LOCAL_SIZE * LOCAL_SIZE * (gl_WorkGroupID.y * gl_NumWorkGroups.x + gl_WorkGroupID.x) + gl_LocalInvocationIndex;
}

void writeReflexionRay(in IndirectLightRay _ray)
{
#if defined(FIRST_RECURSION_STEP)
	g_outReflexionRays[rayIndexFromCoord()] = _ray;
#endif
#if defined(CONTINUE_RECURSION)
	g_outRays[rayIndexFromCoord()] = _ray;
#endif
}

void writeRefractionRay(in IndirectLightRay _ray)
{
#if defined(FIRST_RECURSION_STEP)
	g_outRefractionRays[rayIndexFromCoord()] = _ray;
#endif
#if defined(CONTINUE_RECURSION)
	g_outRays[rayIndexFromCoord()] = _ray;
#endif
}

void computeReflexionRay(uint _objectId, vec3 _lit, vec3 _normal, in Ray _ray, float _t);

void nextBounce(vec3 _lightAbsorbdeByPreBounce, in ClosestHit _closestHit, in Ray _ray, vec3 _eyePos)
{
	uint objId = getObjectId(_closestHit);
	uint matId = getMaterialId(_closestHit);
	float t = _closestHit.t;
	vec3 normal = getHitNormal(_closestHit);
	vec2 uv = getHitUv(_closestHit);
	vec3 P = _ray.from  + _ray.dir * t;
	vec3 albedo = g_BvhMaterialData[matId].color.xyz;
	
#if defined(FIRST_RECURSION_STEP) || defined(CONTINUE_RECURSION)
    if (g_BvhMaterialData[matId].type_ids.x == Material_Mirror)
    {
		vec3 lit = _lightAbsorbdeByPreBounce * albedo * g_BvhMaterialData[matId].params.x;
		computeReflexionRay(objId, lit, normal, _ray, t);
    }
	#if 0
	else if (g_BvhMaterialData[matId].type_ids.x == Material_PBR)
	{
		// Sample pre-filtered specular reflection environment at correct mipmap level.
		//vec3 specularIrradiance = _lit;
		//
		//// Split-sum approximation factors for Cook-Torrance specular BRDF.
		float metalness = g_BvhMaterialData[matId].params.x;
		vec3 Lo = normalize(_eyePos - P);
		float cosLo = max(0.0, dot(normal, Lo));
		vec2 specularBRDF = texture(g_dataTextures[g_TextureBrdf], vec2(cosLo, RoughnessPBR)).xy;
		vec3 F0 = mix(Fdielectric, albedo, metalness);
		vec3 specularIBL = (F0 * specularBRDF.x + specularBRDF.y) * _lightAbsorbdeByPreBounce;

		//vec2 specularBRDF = specularBRDF_LUT.Sample(spBRDF_Sampler, float2(cosLo, roughness)).rg;
		//
		//// Total specular IBL contribution.
		//float3 specularIBL = (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;
		//
		//// Total ambient lighting contribution.
		//ambientLighting = diffuseIBL + specularIBL;
	}
	#endif
	else if(g_BvhMaterialData[matId].type_ids.x == Material_Transparent)
	{
		const float refractionIndice = g_BvhMaterialData[matId].params.y;
		float fresnelReflexion = computeFresnelReflexion(_ray.dir, normal, refractionIndice);

		float objectReflectivity = g_BvhMaterialData[matId].params.x;
		fresnelReflexion = (objectReflectivity + (1.0-objectReflectivity) * fresnelReflexion);

		#if defined(FIRST_RECURSION_STEP)
		computeReflexionRay(objId, _lightAbsorbdeByPreBounce * albedo * fresnelReflexion, normal, _ray, t);
		#endif

		vec3 p = _ray.from + _ray.dir * t;
		vec3 inRefractionRay = refract(_ray.dir, normalize(normal), refractionIndice);
		// inRefractionRay = normalize(inRefractionRay == vec3(0,0,0) ? _ray.dir : inRefractionRay);
		if(inRefractionRay != vec3(0,0,0))
		{
			if(objId < 0xFFFF)
			{
				Ray inRay = createRay(p, inRefractionRay);
				Hit hit;
				if(hitPrimitiveThrough(objId, inRay, 100, hit))
				{
					vec3 outRefractionRay = refract(inRay.dir, -hit.normal, 1.0 / g_BvhMaterialData[matId].params.y);
					outRefractionRay = normalize(outRefractionRay == vec3(0,0,0) ? inRay.dir : outRefractionRay);
					p = inRay.from + inRay.dir * hit.t;
					IndirectLightRay outRay;
					outRay.pos = vec4(p + hit.normal * 0.001, 1);
					outRay.lit = vec4(_lightAbsorbdeByPreBounce * albedo * (1-fresnelReflexion), 0);
					outRay.dir = vec4(outRefractionRay, 0);
					writeRefractionRay(outRay);
				}
			}
		}
	}
#endif

}

void computeReflexionRay(uint _objectId, vec3 _lit, vec3 _normal, in Ray _ray, float _t)
{
	vec3 p = _ray.from + _ray.dir * _t;
	vec3 n = reflect(_ray.dir, _normal);

	IndirectLightRay outRay;
    outRay.pos = vec4(p + _normal * 0.001, 1);
    outRay.lit = vec4(_lit, 0);
    outRay.dir = vec4(normalize(n), 0);
	writeReflexionRay(outRay);
}

#endif