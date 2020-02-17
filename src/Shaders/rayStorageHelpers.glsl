#ifndef H_RAYSTORAGEHELPERS_FXH_
#define H_RAYSTORAGEHELPERS_FXH_

#include "struct_cpp.glsl"

#ifdef CONTINUE_RECURSION
layout(std430, binding = g_OutReflexionRayBuffer_bind) writeonly buffer g_OutReflexionRayBuffer_bind_layout
{
    IndirectLightRay g_outReflexionRays[];
};
#endif

uint rayIndexFromCoord()
{
	return LOCAL_SIZE * LOCAL_SIZE * (gl_WorkGroupID.y * gl_NumWorkGroups.x + gl_WorkGroupID.x) + gl_LocalInvocationIndex;
}

void nextBounce(uint _matId, vec3 _rayLit, vec3 _normal, in Ray _ray, float _t)
{
#ifdef CONTINUE_RECURSION

	uint outRayIndex = rayIndexFromCoord();
    if (g_BvhMaterialData[_matId].type_ids.x == Material_Mirror)
    {
		vec3 p = _ray.from + _ray.dir * _t;
        g_outReflexionRays[outRayIndex].pos = vec4(p + _normal * 0.001, 1);
        g_outReflexionRays[outRayIndex].lit = vec4(g_BvhMaterialData[_matId].color.xyz, 0);

		vec3 n = reflect(_ray.dir, _normal);
		//const float roughness =  0.5;
		//float randAngle = rand(p + vec3(1,2,3))*roughness-roughness*0.5;
		//vec3 randAxis = normalize(vec3(rand(p + vec3(1,0,0)), rand(p+vec3(0,1,0)), rand(p+vec3(0,0,1)))*2-1);
		//vec4 q = rotate_angle_axis( randAngle, normalize(vec3(rand(p + vec3(1,0,0)), rand(p+vec3(0,1,0)), rand(p+vec3(0,0,1)))*2-1) );
		// n += randAxis * 0.1;//rotate_vector(n, q);
        g_outReflexionRays[outRayIndex].dir = vec4(n, 0);
    }
#endif
}

#endif