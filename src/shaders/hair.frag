#version 450
#extension GL_ARB_separate_shader_objects : enable

#define PI 3.14159265359
#define PI_OVER_2 1.57079632679
#define RAD_TO_DEG 57.2957795131
#define DEG_TO_RAD 0.01745329251
#define EPSILON 0.0001

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec3 in_u;
layout(location = 2) in vec3 in_v;
layout(location = 3) in vec3 in_w;
layout(location = 4) in vec3 in_viewDir;
layout(location = 5) in vec3 in_lightDir;

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
	if (vec3Compare(cross(v1, v2), n)) {
		return angle;
	} else {
		return -angle;
	}
}

// sigma is standard deviation
// mu is mean (using a zero-mean gaussian)
float gaussian(float sigma, float x) {
	return ((1.0 / sqrt(2 * PI) * sigma) * exp(-(x) * (x) / 2 * sigma * sigma));
	//return ((1.0 / sqrt(2 * PI) * sigma) * exp(-(x - mu) * (x - mu) / 2 * sigma * sigma));
}

float schlicks(float n, float x) {
	float R0 = ((1.f - n) / (1.f + n)) * ((1.f - n) / (1.f + n));
	return R0 + (1.f - R0) * pow(1.f - x, 5);
}

float schlicks(float n1, float n2, float theta) {
	float R0 = ((n1 - n2) / (n1 + n2)) * ((n1 - n2) / (n1 + n2));
	return R0 + (1.f - R0) * pow(1.f - cos(theta), 5);
}

// Fresnel equation
float F(float n_p, float n_pp, float gamma) {
	// TODO: not schlicks???
	return schlicks(n_p, n_pp, gamma);
}

// should it be h or gamma_t?
// accounts for volume absoprtion
vec3 T(vec3 sigma, float gamma_t) {
	return exp(-2.f * sigma * cos(gamma_t));
}

vec3 A(float p, float h, float n, vec3 sigma_a) {
	float theta_t = 0.f; // TODO?

	float gamma_i = asin(h);

	float n_p = sqrt(n * n - sin(gamma_i) * sin(gamma_i)) / cos(gamma_i);
	float n_pp = n * n * cos(gamma_i) / sqrt(n * n - sin(gamma_i) * sin(gamma_i));

	float gamma_t = asin(h / n_p);
	
	if (p == 0) {
		return F(n_p, n_pp, gamma_i) * vec3(1.f); 
	} else {
		vec3 T = T(sigma_a / cos(theta_t), gamma_t);
		vec3 Tpow = vec3(pow(T.r, p), pow(T.g, p), pow(T.b, p));
		return (1.f - F(n_p, n_pp, gamma_i)) * (1.f - F(n_p, n_pp, gamma_i)) * pow(F(1.f / n_p, 1.f / n_pp, gamma_t), p - 1) * Tpow;
	}
}

float N(float p, float phi) {
	int numRoots = 1;

	float N = 0;
	//for (int i = 0; i < numRoots; i++) {
	//	N += A(p, h(p, r, phi)) * abs(dphi_dh);
	//}
	return N;
}

void main() {
	const float over255 = 1.0 / 255.0;

	const vec3 w_i = in_lightDir;
	const vec3 w_o = in_viewDir;

	// TODO: make these globals?
	const float n = 1.55; // index of refraction
	const vec3 sigma_a = vec3(0.132, 0.212, 0.78); // absorption coeffcient
	const float a = 0.85; // eccentricity: [0.85, 1]

	// all angles in degrees
	const float theta_i = angleBetweenVectorAndPlane(w_i, in_u) * RAD_TO_DEG;
	const float theta_o = angleBetweenVectorAndPlane(w_o, in_u) * RAD_TO_DEG;

	//const float phi_i = angleBetweenVectorAndPlane(w_i, in_v) * RAD_TO_DEG;
	const float phi_i = getDirectionalAngleBetweenVectors(projectVectorOntoPlane(w_i, in_u), in_v, in_u) * RAD_TO_DEG;
	//const float phi_o = angleBetweenVectorAndPlane(w_o, in_v) * RAD_TO_DEG;
	const float phi_o = getDirectionalAngleBetweenVectors(projectVectorOntoPlane(w_o, in_u), in_v, in_u) * RAD_TO_DEG;

	const float theta_h = (theta_i + theta_o) / 2.f;
	const float theta_d = (theta_o - theta_i) / 2.f;

	const float phi_h = (phi_o + phi_i) / 2.f;
	const float phi_d = (phi_o - phi_i) / 2.f;
	const float phi = phi_o - phi_i;
	//float phi = phi_d;


	const float alpha_R = -5.f; // -10 to -5 degrees
	const float alpha_TT = -alpha_R / 2.0;
	const float alpha_TRT = -3.f * alpha_R / 2.f;
	const float beta_R = 10.0; // 5 to 10 degrees
	const float beta_TT = 5.f;//beta_R / 2.0;
	const float beta_TRT = 20.f;//2.f * beta_R;

	const float beta_N = 0.3; // roughness TODO?

	// longitudinal scattering function M
	float M_R = gaussian(beta_R, theta_h - alpha_R);
	float M_TT = gaussian(beta_TT, theta_h - alpha_TT);
	float M_TRT = gaussian(beta_TRT, theta_h - alpha_TRT);

	// azimuthal scattering function N
	// using Frostbite engine approximations (SIGGRAPH hair course 2019)
	vec3 N_R = schlicks(n, sqrt(0.5 * (1.f + dot(w_i, w_o)))) * 0.25f * cos(phi_d / 2.f * DEG_TO_RAD) * vec3(1.f);
	vec3 N_TT = A(1, 0, n, sigma_a) * 1.f; // TODO: missing term
	//float s_r = clamp(1.5 * (1.f - beta_N), 0.f, 1.f);
	float s_r = 1.5 * (1.f - beta_N);
	vec3 N_TRT = A(2, sqrt(3.f) / 2.f, n, sigma_a) * s_r * exp(s_r * (17.f * cos(phi * DEG_TO_RAD) - 16.78f));

	// artist approximations
	float N_R2 = cos(phi_d / 2.f * DEG_TO_RAD);
	float gamma_TT = 5;
	float gamma_g = 10;
	float I_g = 5.f;
	float N_TT2 = gaussian(PI - phi_d, gamma_TT * gamma_TT);
	float N_TRT2 = N_R2 + I_g * gaussian(35.f - phi_d, gamma_g * gamma_g); 

	// scattering function S
	float cosTheta_d = cos(theta_d * DEG_TO_RAD);
	vec3 S = (M_R * N_R + M_TT * N_TT + M_TRT * N_TRT) / cosTheta_d * cosTheta_d; 
	float S3 = (M_R + M_TT + M_TRT) / cosTheta_d * cosTheta_d; 
	vec3 S2 = vec3(227, 204, 236) * over255 * (M_R * N_R2 + M_TT * N_TT2 + M_TRT * N_TRT2) / cosTheta_d * cosTheta_d; 

	// gradient color
	vec3 c1 = vec3(171, 146, 99) * over255;
	vec3 c2 = vec3(216,192,120) * over255;
	vec3 c3 = vec3(242,231,199) * over255;
	float t = in_uv.y;
	vec3 color = mix(mix(c1, c2, t), mix(c2, c3, t), t);

	//color = vec3(25, 16, 8) * over255;
	//theta_i += 180;
    //outColor = vec4(N_TRT, N_TRT, N_TRT, 1.0);
	outColor = vec4(0.25f * cos(phi_d / 2.f * DEG_TO_RAD) * vec3(1.f), 1.0);
	outColor = vec4(projectVectorOntoPlane(w_i, in_u), 1.0);
	outColor = vec4(S2, 1.0);
	//outColor = vec4(color, 1.0);
}
