#version 450
#extension GL_ARB_separate_shader_objects : enable

#define PI 3.14159265359
#define PI_OVER_2 1.57079632679
#define RAD_TO_DEG 57.2957795131
#define DEG_TO_RAD 0.01745329251
#define EPSILON 0.001

layout(set = 1, binding = 1) uniform sampler2D depthSampler;

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
float gaussian(float sigma, float x) {
	return ((1.0 / sqrt(2 * PI) * sigma) * exp(-(x * x) / 2 * sigma * sigma));
	//return ((1.0 / sqrt(2 * PI) * sigma) * exp(-(x - mu) * (x - mu) / 2 * sigma * sigma));
}

float gaussian2(float beta, float theta_i, float theta_o, float I_0) {
	float v = beta * beta;
	return (1.f / ((v * exp(2.f / v)) - v)) * 
		exp((1.f - sin(theta_i * DEG_TO_RAD) * sin(theta_o * DEG_TO_RAD)) / v) * I_0 * (cos(theta_i * DEG_TO_RAD) * cos(theta_o * DEG_TO_RAD) / v);
}


float gaussian3(float alpha, float beta, float theta_i, float theta_o) {
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

float shadowCalculation(vec4 fragPosLightSpace) {
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords.xy = projCoords.xy * 0.5 + 0.5;
	float closestDepth = texture(depthSampler, projCoords.xy).r;
	float currentDepth = projCoords.z;
	float shadow = currentDepth > closestDepth ? 1.0 : 0.0;
	return shadow;
	//return currentDepth;
}

void main() {
	const float over255 = 1.0 / 255.0;

	const vec3 w_i = normalize(in_lightDir);
	const vec3 w_o = normalize(in_viewDir);

	// TODO: make these globals?
	const float n = 1.55f; // index of refraction
	const vec3 sigma_a = vec3(0.132, 0.212, 0.78); // absorption coeffcient
	const float eccentricity = 0.85; // eccentricity: [0.85, 1]
	const vec3 C = vec3(89, 38, 11) * over255;
	//const vec3 C = vec3(216,192,120) * over255;
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

	//const float phi_i = angleBetweenVectorAndPlane(w_i, in_v) * RAD_TO_DEG;
	const float phi_i = getDirectionalAngleBetweenVectors(normalize(projectVectorOntoPlane(w_i, in_u)), in_v, in_u) * RAD_TO_DEG;
	//const float phi_o = angleBetweenVectorAndPlane(w_o, in_v) * RAD_TO_DEG;
	const float phi_o = getDirectionalAngleBetweenVectors(normalize(projectVectorOntoPlane(w_o, in_u)), in_v, in_u) * RAD_TO_DEG;

	float theta_h = (theta_i + theta_o) / 2.f;
	theta_h = (asin(lightVec) + asin(viewDepSpec)) *RAD_TO_DEG;

	const float theta_d = (theta_o - theta_i) / 2.f;

	const float phi_h = (phi_o + phi_i) / 2.f;
	const float phi_d = phi_o - phi_i;


//	const float alpha_R = -5.f; // -10 to -5 degrees
//	const float alpha_TT = -alpha_R / 2.0;
//	const float alpha_TRT = -3.f * alpha_R / 2.f;
//	const float beta_R = 5.0; // 5 to 10 degrees
//	const float beta_TT = beta_R / 2.0;
//	const float beta_TRT = 2.f * beta_R;
	const float alpha_R = -2.f * shift;
	const float alpha_TT = shift;
	const float alpha_TRT = 4.f * shift;
	const float beta_R = roughness * roughness;
	const float beta_TT = 0.5 * beta_R;
	const float beta_TRT = 2.f * beta_R;

	const float beta_N = 0.3; // roughness TODO?

	// longitudinal scattering function M
	float M_R = gaussian(beta_R, theta_h - alpha_R);
	float M_TT = gaussian(beta_TT, theta_h - alpha_TT);
	float M_TRT = gaussian(beta_TRT, theta_h - alpha_TRT);

	float M_R_2 = gaussian2(0.8, theta_i, theta_o, 0.9);
	float M_TT_2 = gaussian2(beta_TT, theta_i, theta_o, 0.9);
	float M_TRT_2 = gaussian2(beta_TRT, theta_i, theta_o, 0.9);

	float M_R_3 = gaussian3(alpha_R, beta_R, theta_i, theta_o);
	float M_TT_3 = gaussian3(alpha_TT, beta_TT, theta_i, theta_o); // smaller beta is more defined
	float M_TRT_3 = gaussian3(alpha_TRT, beta_TRT, theta_i, theta_o);

	float cosTheta_d = cos(theta_d * DEG_TO_RAD);
	float cosPhi_d = cos(phi_d * DEG_TO_RAD);
	cosPhi_d = dot(normalize(in_lightDir - lightVec * in_u), normalize(in_viewDir - viewDepSpec * in_u));

	float cosPhi_dOver2 = cos((phi_d / 2.f) * DEG_TO_RAD);

	// azimuthal scattering function N
	// using Unreal engine approximations
	//vec3 N_R = schlicks(n, sqrt(0.5f + 0.5f * dot(w_i, w_o))) * (0.25f * cos((phi_d / 2.f) * DEG_TO_RAD)) * vec3(1.f);
	vec3 N_R = schlicks(n, sqrt(0.5f + 0.5f * dot(w_i, w_o)))*  (0.25f * sqrt(0.5f + 0.5f * cosPhi_d)) * vec3(1.f);

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

	float h_TRT = sqrt(3.f) / 2.f;
	float f_TRT = schlicks(n, cosTheta_d * sqrt(1.f - h_TRT * h_TRT));
	float Cpower_TRT = 0.8f / cosTheta_d;
	vec3 T_TRT = vec3(pow(C.r, Cpower_TRT), pow(C.g, Cpower_TRT), pow(C.b, Cpower_TRT));
	vec3 A_TRT = (1.f - f_TRT) * (1.f - f_TRT) * f_TRT * T_TRT * T_TRT; 
	float D_TRT = exp(17.f * cosPhi_d - 16.78f);
	vec3 N_TRT = A_TT * D_TT;

	// artist approximations
	float M_R_art = gaussianPrime((theta_h + 5), 2.f * 2.f);
	float M_TT_art = gaussianPrime((theta_h + 25.f), 1.f * 1.f);
	float M_TRT_art = gaussianPrime((theta_h + 5.f), 2.f * 2.f);
	float N_R_art = cosPhi_dOver2;
	float gamma_TT = 2;
	float gamma_g = 10;
	float I_g = 5.f; // intensity
	float N_TT_art = gaussianPrime(180.f - phi_d, gamma_TT * gamma_TT);
	float N_TRT_art = cosPhi_dOver2 + I_g * gaussianPrime(35.f - phi_d, gamma_g * gamma_g); 
	vec3 C_R = vec3(242,231,199) * over255;
	vec3 C_TT = vec3(216,192,120) * over255;
	vec3 C_TRT = vec3(171, 146, 99) * over255;
	float I_R = 0.5f;
	float I_TT = 0.5f;
	float I_TRT = 1.f;
	vec3 S_art = (M_R_art * N_R_art * I_R * C_R +
				  M_TT_art * N_TRT_art * I_TT * C_TT +
				  M_TRT_art * N_TRT_art * I_TRT * C_TRT
				  ) / (cosTheta_d * cosTheta_d); 

	// scattering function S
	vec3 S_TT = (N_TT * M_TT_3) / (cosTheta_d * cosTheta_d); 
	vec3 S_R = (N_R * M_R_3) / (cosTheta_d * cosTheta_d); 
	vec3 S = (N_R * M_R_3 + N_TT * M_TT_3 + N_TRT * M_TRT_3) / (cosTheta_d * cosTheta_d); 

	// gradient color
	vec3 c1 = vec3(171, 146, 99) * over255;
	vec3 c2 = vec3(216,192,120) * over255;
	vec3 c3 = vec3(242,231,199) * over255;
	float t = in_uv.y;
	vec3 color = mix(mix(c1, c2, t), mix(c2, c3, t), t);

	//float test = getDirectionalAngleBetweenVectors(normalize(projectVectorOntoPlane(w_o, in_u)), in_v, in_u) / (2 * PI);
	//outColor = vec4(test, test, test, 1.0);
	
	//outColor = vec4(S, 1.0);

	vec3 test = (vec3(texture(depthSampler, in_uv)) - 0.75f) * 4.f;

	float shadow = shadowCalculation(in_fragPosLightSpace);
	vec3 colorTest = vec3(shadow);

	outColor = vec4(S * (1.f - shadow), 1.0);
}
