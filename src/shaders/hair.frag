#version 450
#extension GL_ARB_separate_shader_objects : enable

#define PI 3.14159265359
#define PI_OVER_2 1.57079632679
#define RAD_TO_DEG 57.2957795131
#define DEG_TO_RAD 0.01745329251
#define EPSILON 0.001

layout(set = 1, binding = 1) uniform sampler2D depthSampler;
layout(set = 3, binding = 0) uniform sampler2D opacitySampler;

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec3 in_u;
layout(location = 2) in vec3 in_v;
layout(location = 3) in vec3 in_w;
layout(location = 4) in vec3 in_viewDir;
layout(location = 5) in vec3 in_lightDir;
layout(location = 6) in vec4 in_fragPosLightSpace;

layout(location = 0) out vec4 outColor;

bool vec3Compare(vec3 v1, vec3 v2) {
	return (abs(v1.x - v2.x) < EPSILON &&
			abs(v1.y - v2.y) < EPSILON &&
			abs(v1.z - v2.z) < EPSILON);
}

// returns angle in radians
float angleBetweenVectorAndPlane(vec3 vec, vec3 n) {
	float angle = acos(dot(vec, n) / (length(vec) * length(n)));
	if (angle < PI_OVER_2) {
		angle = PI_OVER_2 - angle;
	} else {
		angle = -(angle - PI_OVER_2);
	}
	return angle;
}

vec3 projectVectorOntoPlane(vec3 vec, vec3 n) {
	return vec - dot(vec, n) * n;
}

float getDirectionalAngleBetweenVectors(vec3 v1, vec3 v2, vec3 n) {
	float angle = acos(dot(v1, v2) / (length(v1) * length(v2)));

	if (vec3Compare(normalize(cross(v2, v1)), n)) {
		return angle; // range 0 to PI
	} else {
		return (2 * PI) - angle; // range PI to PI/2 
	}
	return angle;

}

vec3 transformVectorCoordinateSystem(vec3 vec, vec3 x_, vec3 y_, vec3 z_) {
	vec3 x = vec3(1, 0, 0);
	vec3 y = vec3(0, 1, 0);
	vec3 z = vec3(0, 0, 1);
	mat3 transform = mat3(1);

	transform[0][0] = dot(x_, x); 
	transform[0][1] = dot(x_, y); 
	transform[0][2] = dot(x_, z); 

	transform[1][0] = dot(y_, x); 
	transform[1][1] = dot(y_, y); 
	transform[1][2] = dot(y_, z); 

	transform[2][0] = dot(z_, x); 
	transform[2][1] = dot(z_, y); 
	transform[2][2] = dot(z_, z); 

	return transform * vec;
}

// sigma is standard deviation
// mu is mean (using a zero-mean gaussian)
float gaussian(float alpha, float beta, float theta_i, float theta_o) {
	return (1.f / (beta * sqrt(2.f * PI))) * exp(-pow(sin(theta_i * DEG_TO_RAD) + sin(theta_o * DEG_TO_RAD) - alpha, 2) / (2.f * beta * beta));
}

float gaussianPrime(float v, float x) {
	return exp((-PI * v * v) / (2.f * x * x));
}

float schlicks(float n, float x) {
	//float R0 = ((1.f - n) / (1.f + n)) * ((1.f - n) / (1.f + n));
	float R0 = ((1.f - n) * (1.f - n)) / ((1.f + n) * (1.f + n));
	return R0 + (1.f - R0) * pow(1.f - x, 5);
}

float schlicks(float n1, float n2, float theta) {
	float R0 = ((n1 - n2) / (n1 + n2)) * ((n1 - n2) / (n1 + n2));
	return R0 + (1.f - R0) * pow(1.f - cos(theta), 5);
}

float PCFshadowCalculation(vec4 fragPosLightSpace) {
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

	if (abs(projCoords.x) > 1.f || abs(projCoords.y) > 1.f || abs(projCoords.z) > 1.f ) {
		return 0.f;
	}
	projCoords.xy = projCoords.xy * 0.5 + 0.5;
	float currentDepth = projCoords.z;

	int pcfSize = 4;
	int pcfSizeMinus1 = int(pcfSize - 1);
	float kernelSize = 2.f * pcfSizeMinus1 + 1.f;
	float numSamples = kernelSize * kernelSize;
	
	float lightedCount = 0.f;
	vec2 shadowMapSize = vec2(1080, 720);
	vec2 shadowMapTexelSize = 1.f / shadowMapSize;
	for (int x = -pcfSizeMinus1; x <= pcfSizeMinus1; x++) {
		for (int y = -pcfSizeMinus1; y <= pcfSizeMinus1; y++) {
			vec2 pcfCoords = projCoords.xy + vec2(x, y) * shadowMapTexelSize;
			float closestDepth = texture(depthSampler, pcfCoords.xy).r;
			if (currentDepth > closestDepth) {
				lightedCount += 1.f;
			}
		}
	}
	return lightedCount / numSamples;
	//float closestDepth = texture(depthSampler, projCoords.xy).r;
	//float currentDepth = projCoords.z;
	//return currentDepth > closestDepth ? 0.f : 1.f;
}

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

void main() {
	const float over255 = 1.0 / 255.0;

	const vec3 w_i = normalize(in_lightDir);
	const vec3 w_o = normalize(in_viewDir);

	// TODO: make these globals?
	const float n = 1.55f; // index of refraction
	const vec3 sigma_a = vec3(0.132, 0.212, 0.78); // absorption coeffcient
	const float eccentricity = 0.85; // eccentricity: [0.85, 1]
	//vec3 C = vec3(213, 124, 40) * over255;
	vec3 C = vec3(216,192,120) * over255;
	const float roughness = 0.2;
	const float shift = 0.005;

	vec3 w_i_trans = normalize(transformVectorCoordinateSystem(w_i, in_v, in_u, in_w));
	vec3 w_o_trans = normalize(transformVectorCoordinateSystem(w_o, in_v, in_u, in_w));
	
	float theta_i_trans = acos(w_i_trans.z / 1.f) * RAD_TO_DEG;
	float theta_o_trans = acos(w_o_trans.z / 1.f) * RAD_TO_DEG;

	float lightVec = dot(in_lightDir, in_u);
	float viewDepSpec = dot(in_viewDir, in_u);

	// all angles in degrees
	const float theta_i = angleBetweenVectorAndPlane(w_i, in_u) * RAD_TO_DEG;
	const float theta_o = angleBetweenVectorAndPlane(w_o, in_u) * RAD_TO_DEG;

	const float phi_i = getDirectionalAngleBetweenVectors(normalize(projectVectorOntoPlane(w_i, in_u)), in_v, in_u) * RAD_TO_DEG;
	const float phi_o = getDirectionalAngleBetweenVectors(normalize(projectVectorOntoPlane(w_o, in_u)), in_v, in_u) * RAD_TO_DEG;

	float theta_h = (theta_i + theta_o) / 2.f;
	//theta_h = (asin(lightVec) + asin(viewDepSpec)) * RAD_TO_DEG;

	const float theta_d = (theta_o - theta_i) / 2.f;

	const float phi_h = (phi_o + phi_i) / 2.f;
	const float phi_d = phi_o - phi_i;

	const float alpha_R = -2.f * shift;
	const float alpha_TT = shift;
	const float alpha_TRT = 4.f * shift;
	const float beta_R = roughness * roughness;
	const float beta_TT = 0.5 * beta_R;
	const float beta_TRT = 2.f * beta_R;

	const float beta_N = 0.3; // roughness TODO?

	// longitudinal scattering function M
	float M_R = gaussian(alpha_R, beta_R, theta_i, theta_o);
	float M_TT = gaussian(alpha_TT, beta_TT, theta_i, theta_o); // smaller beta is more defined
	float M_TRT = gaussian(alpha_TRT, beta_TRT, theta_i, theta_o);

	float cosTheta_d = cos(theta_d * DEG_TO_RAD);
	float cosPhi_d = cos(phi_d * DEG_TO_RAD);
	//cosPhi_d = dot(normalize(in_lightDir - lightVec * in_u), normalize(in_viewDir - viewDepSpec * in_u));

	float cosPhi_dOver2 = cos((phi_d / 2.f) * DEG_TO_RAD);

	// azimuthal scattering function N, using Unreal engine approximations
	// azimuthal R
	//vec3 N_R = schlicks(n, sqrt(0.5f + 0.5f * dot(w_i, w_o))) * (0.25f * cos((phi_d / 2.f) * DEG_TO_RAD)) * vec3(1.f);
	vec3 N_R = schlicks(n, sqrt(0.5f + 0.5f * dot(w_i, w_o)))*  (0.25f * sqrt(0.5f + 0.5f * cosPhi_d)) * vec3(1.f);

	// azimuthal TT
	float n_prime = (1.19f / cosTheta_d) + 0.36 * cosTheta_d;
	float a = 1.f / n_prime;
	float h_TT = (1.f + a * (0.6 - 0.8 * cosPhi_d)) * cosPhi_dOver2;
	float f_TT = schlicks(n, cosTheta_d * sqrt(1.f - h_TT * h_TT));
	float gamma_t = asin(h_TT / n_prime);
	float Cpower_TT = sqrt(1.f - h_TT * h_TT * a * a) / (2.f * cosTheta_d);
	vec3 eta_TT = vec3(pow(C.r, Cpower_TT), pow(C.g, Cpower_TT), pow(C.b, Cpower_TT));
	//vec3 T_TT = exp(-eta_TT * (cos(gamma_t) / cosTheta_d));
	vec3 T_TT = eta_TT;
	vec3 A_TT = (1.f - f_TT) * (1.f - f_TT) * T_TT; 
	float D_TT = exp(-3.65f * cosPhi_d - 3.98f);
	vec3 N_TT = A_TT * D_TT;

	// azimuthal TRT
	float h_TRT = sqrt(3.f) / 2.f;
	float f_TRT = schlicks(n, cosTheta_d * sqrt(1.f - h_TRT * h_TRT));
	float Cpower_TRT = 0.8f / cosTheta_d;
	vec3 T_TRT = vec3(pow(C.r, Cpower_TRT), pow(C.g, Cpower_TRT), pow(C.b, Cpower_TRT));
	vec3 A_TRT = (1.f - f_TRT) * (1.f - f_TRT) * f_TRT * T_TRT * T_TRT; 
	float D_TRT = exp(17.f * cosPhi_d - 16.78f);
	vec3 N_TRT = A_TT * D_TT;

	// single scattering function S
	vec3 S_single = (N_R * M_R + 0.5 * N_TT * M_TT + 0.5 * N_TRT * M_TRT) / (cosTheta_d * cosTheta_d); 

	// gradient color
	vec3 c1 = vec3(171, 146, 99) * over255;
	vec3 c2 = vec3(216,192,120) * over255;
	vec3 c3 = vec3(242,231,199) * over255;
	float t = in_uv.y;
	vec3 color = mix(mix(c1, c2, t), mix(c2, c3, t), t);
	
	float shadow = PCFshadowCalculation(in_fragPosLightSpace);

	vec3 projCoords = in_fragPosLightSpace.xyz / in_fragPosLightSpace.w;
	projCoords.xy = projCoords.xy * 0.5 + 0.5;
	float z0 = texture(depthSampler, projCoords.xy).r;
	//z0 = texture(opacitySampler, projCoords.xy).a;
	float currentDepth = projCoords.z;

	const float d = 0.3;

	float opacity = 0.f;
	if (currentDepth >= z0 && currentDepth < z0 + d) {
		opacity = texture(opacitySampler, projCoords.xy).r;
	} else if (currentDepth >= z0 + d && currentDepth < z0 + 2.f * d) {
		opacity = texture(opacitySampler, projCoords.xy).g;
	} else if (currentDepth >= z0 + 2.f * d) { 
		opacity = texture(opacitySampler, projCoords.xy).b;
	}

	// multiple scattering
	vec3 fakeNormal = normalize(w_o - in_u * dot(in_u, w_o));
	float luma = 0.3f;
	vec3 Cexp = vec3(pow(C.r / luma, 1.f - opacity), pow(C.g / luma, 1.f - opacity), pow(C.b / luma, 1.f - opacity));
	vec3 S_multi = sqrt(C) * ((dot(fakeNormal, w_i) + 1.f) / (4.f * PI)) * Cexp;

	//outColor = vec4(S * (1.f - opacity), 1.0);
	outColor = vec4(clamp(((S_single + S_multi) * (1.f - shadow) + 0.3 * C) * (0.5 * abs(random(vec2(in_uv.x, in_uv.x))) + 0.75), 0.f, 1.f), 0.75);
}
