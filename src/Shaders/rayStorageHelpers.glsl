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

void nextBounce(uint _objectId, uint _matId, vec3 _lightAbsorbdeByPreBounce, vec3 _normal, in Ray _ray, float _t)
{
#if defined(FIRST_RECURSION_STEP) || defined(CONTINUE_RECURSION)
    if (g_BvhMaterialData[_matId].type_ids.x == Material_Mirror)
    {
		vec3 lit = _lightAbsorbdeByPreBounce * g_BvhMaterialData[_matId].color.xyz * g_BvhMaterialData[_matId].params.x;
		computeReflexionRay(_objectId, lit, _normal, _ray, _t);
    }
#endif

#if defined(FIRST_RECURSION_STEP) || defined(CONTINUE_RECURSION)
	if(g_BvhMaterialData[_matId].type_ids.x == Material_Transparent)
	{
		const float refractionIndice = g_BvhMaterialData[_matId].params.y;
		float fresnelReflexion = computeFresnelReflexion(_ray.dir, _normal, refractionIndice);

		float objectReflectivity = g_BvhMaterialData[_matId].params.x;
		fresnelReflexion = (objectReflectivity + (1.0-objectReflectivity) * fresnelReflexion);

		#if defined(FIRST_RECURSION_STEP)
		computeReflexionRay(_objectId, _lightAbsorbdeByPreBounce * g_BvhMaterialData[_matId].color.xyz * fresnelReflexion, _normal, _ray, _t);
		#endif

		vec3 p = _ray.from + _ray.dir * _t;
		vec3 inRefractionRay = refract(_ray.dir, normalize(_normal), refractionIndice);
		// inRefractionRay = normalize(inRefractionRay == vec3(0,0,0) ? _ray.dir : inRefractionRay);
		if(inRefractionRay != vec3(0,0,0))
		{
			Ray inRay = createRay(p, inRefractionRay);
			Hit hit;
			if(hitPrimitiveThrough(_objectId, inRay, 100, hit))
			{
				vec3 outRefractionRay = refract(inRay.dir, -hit.normal, 1.0 / g_BvhMaterialData[_matId].params.y);
				outRefractionRay = normalize(outRefractionRay == vec3(0,0,0) ? inRay.dir : outRefractionRay);
				p = inRay.from + inRay.dir * hit.t;
				IndirectLightRay outRay;
				outRay.pos = vec4(p + hit.normal * 0.001, 1);
				outRay.lit = vec4(_lightAbsorbdeByPreBounce * g_BvhMaterialData[_matId].color.xyz * (1-fresnelReflexion), 0);
				outRay.dir = vec4(outRefractionRay, 0);
				writeRefractionRay(outRay);
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