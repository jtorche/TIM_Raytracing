#ifndef H_MATH_FXH_
#define H_MATH_FXH_

float rand(float n)
{ 
	return fract(sin(n * 12.9898) * 43758.5453123) * 0.5 + 0.5; 
}

float rand(vec2 co)
{
    float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    float dt = dot(co.xy, vec2(a,b));
    float sn = mod(dt,3.14);
    return fract(sin(sn) * c);
}

float rand(vec3 co)
{
    float a = 12.9898;
    float b = 78.233;
	float c = 57.4535;
    float d = 43758.5453;
    float dt = dot(co, vec3(a,b,c));
    float sn = mod(dt,3.14);
    return fract(sin(sn) * d) * 0.5 + 0.5;
}

//float rand(vec2 co)
//{
//   return fract(sin(dot(co.xy,vec2(12.9898,78.233))) * 43758.5453);
//}

vec3 sampleSphere(vec2 rands)
{
	float theta = 2 * 3.14159 * rands.x;
    float phi = acos(1 - 2 * rands.y);
    return vec3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));
}

vec3 refractRay(vec3 D, vec3 N, float n)
{
	float cos_i = dot(N,-D);
	float sin_t2 = n*n*(1-cos_i*cos_i);
	return n*D + (n*cos_i - sqrt(1-sin_t2))*N;
}

float computeFresnelReflexion(vec3 I, vec3 N, float refractionIndex) 
{ 
    float cosi = clamp(dot(I, N), -1, 1); 
    float etai = 1, etat = refractionIndex; 
    if (cosi > 0) 
	{ 
		etai=refractionIndex; 
		etat=1; 
	} 
    // Compute sini using Snell's law
    float sint = etai / etat * sqrt(max(0, 1.0 - cosi * cosi)); 
    // Total internal reflection

    float kr;

    if (sint >= 1)
        kr = 1;
    else 
	{ 
        float cost = sqrt(max(0, 1.0 - sint * sint)); 
        cosi = abs(cosi); 
        float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost)); 
        float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost)); 
        kr = (Rs * Rs + Rp * Rp) / 2; 
    } 

    // As a consequence of the conservation of energy, transmittance is given by:
    // kt = 1 - kr;

    return kr;
} 

// Quaternion multiplication
// http://mathworld.wolfram.com/Quaternion.html
vec4 qmul(vec4 q1, vec4 q2) {
	return vec4(
		q2.xyz * q1.w + q1.xyz * q2.w + cross(q1.xyz, q2.xyz),
		q1.w * q2.w - dot(q1.xyz, q2.xyz)
	);
}

// Vector rotation with a quaternion
// http://mathworld.wolfram.com/Quaternion.html
vec3 rotate_vector(vec3 v, vec4 r) {
	vec4 r_c = r * vec4(-1, -1, -1, 1);
	return qmul(r, qmul(vec4(v, 0), r_c)).xyz;
}

// A given angle of rotation about a given axis
vec4 rotate_angle_axis(float angle, vec3 axis) {
	float sn = sin(angle * 0.5);
	float cs = cos(angle * 0.5);
	return vec4(axis * sn, cs);
}

#endif