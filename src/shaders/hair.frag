#version 450
#extension GL_ARB_separate_shader_objects : enable

#define PI 3.14159265359
#define PI_OVER_2 1.57079632679

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec3 in_u;
layout(location = 2) in vec3 in_v;
layout(location = 3) in vec3 in_w;
layout(location = 4) in vec3 in_viewDir;
layout(location = 5) in vec3 in_lightDir;

layout(location = 0) out vec4 outColor;

float angleBetweenVectorAndPlane(vec3 vec, vec3 n) {
	float angle = acos(dot(vec, n) / length(vec) * length(n));
	if (angle < PI_OVER_2) {
		angle = PI_OVER_2 - angle;
	} else {
		angle = angle - PI_OVER_2;
	}
	return angle;
}

// sigma is standard deviation
// mu is mean (using a zero-mean gaussian)
float gaussian(float sigma, float x) {
	return ((1.0 / sqrt(2 * PI) * sigma) * exp(-(x) * (x) / 2 * sigma * sigma));
	//return ((1.0 / sqrt(2 * PI) * sigma) * exp(-(x - mu) * (x - mu) / 2 * sigma * sigma));
}

// Fresnel equation
float F(float n1, float n2, float gamma) {
	return 1.f;
	// ???????
}

// should it be h or gamma_t?
// accounts for volume absoprtion
float T(float sigma, float gamma_t) {
	return exp(-2.f * sigma * (1.f + cos(gamma_t)));
}

float A(float p, float h) {
	float n1;
	float n2;
	float gamma_i;
	float gamma_t;
	float sigma_a_prime;
	
	if (p == 0) {
		return F(n1, n2, gamma_i); 
	} else {
		return (1.f - F(n1, n2, gamma_i)) * (1.f - F(n1, n2, gamma_i)) * pow(F(1.f / n1, 1.f / n2, gamma_t), p - 1) * pow(T(sigma_a_prime, h), p);
	}
}

float N(float p, float phi) {
	int numRoots = 1;

	float N = 0;
	for (int i = 0; i < numRoots; i++) {
		N += A(p, h(p, r, phi)) * abs(dphi_dh);
	}
	return N;
}

void main() {
	float over255 = 1.0 / 255.0;


	vec3 w_i = in_lightDir;
	vec3 w_o = in_viewDir;

	// all angles in degrees
	float theta_i = angleBetweenVectorAndPlane(w_i, in_u) * 180.0 / PI;
	float theta_o = angleBetweenVectorAndPlane(w_o, in_u) * 180.0 / PI;

	float theta_h = (theta_i + theta_o) / 2.f;
	float theta_d = (theta_o - theta_i) / 2.f;

	float phi;

	float alpha_R = -10; // -10 to -5 degrees
	float alpha_TT = -alpha_R / 2.0;
	float alpha_TRT = - 3.0 * alpha_R / 2.0;
	float beta_R = 5.0; // 5 to 10 degrees
	float beta_TT = beta_R / 2.0;
	float beta_TRT = 2 * beta_R;

	// longitudinal scattering function M
	float M_R = gaussian(beta_R, theta_h - alpha_R);
	float M_TT = gaussian(beta_TT, theta_h - alpha_TT);
	float M_TRT = gaussian(beta_TRT, theta_h - alpha_TRT);

	// azimuthal scattering function N
	float N_R = N(0, phi);
	float N_TT = N(1, phi);
	float N_TRT = 1.f;

	// scattering function S
	float S = (M_R * N_R + M_TT * N_TT + M_TRT * N_TRT) / cos(theta_d) * cos(theta_d); 

	// gradient color
	vec3 c1 = vec3(171, 146, 99) * over255;
	vec3 c2 = vec3(216,192,120) * over255;
	vec3 c3 = vec3(242,231,199) * over255;

	float t = in_uv.y;

	vec3 color = mix(mix(c1, c2, t), mix(c2, c3, t), t);
	//color = vec3(25, 16, 8) * over255;
    outColor = vec4(S, S, S, 1.0);
    //outColor = vec4(S, S, S, 1.0);
	
	//outColor = vec4(0.96, 0.89, 0.56, 1.0);
}
