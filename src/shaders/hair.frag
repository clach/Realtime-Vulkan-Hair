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
// mu is mean
float gaussian(float sigma, float mu, float x) {
	return ((1.0 / sqrt(2 * PI) * sigma) * exp(-(x - mu) * (x - mu) / 2 * sigma * sigma));
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

	float alpha_R = -10; // -10 to -5 degrees
	float alpha_TT = -alpha_R / 2.0;
	float alpha_TRT = - 3.0 * alpha_R / 2.0;
	float beta_R = 5.0; // 5 to 10 degrees
	float beta_TT = beta_R / 2.0;
	float beta_TRT = 2 * beta_R;

	// TODO...what is mu?
	// longitudinal scattering function M
	float M_R = gaussian(beta_R, alpha_R, theta_h);
	float M_TT = gaussian(beta_TT, alpha_TT, theta_h);
	float M_TRT = gaussian(beta_TRT, alpha_TRT, theta_h);

	// azimuthal scattering function N
	float N_R = 1.f;
	float N_TT = 1.f;
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
