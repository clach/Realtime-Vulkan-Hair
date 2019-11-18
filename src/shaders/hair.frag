#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 outColor;

void main() {
	float over255 = 1.0 / 255.0;

	// gradient color
	vec3 c1 = vec3(171, 146, 99) * over255;
	vec3 c2 = vec3(216,192,120) * over255;
	vec3 c3 = vec3(242,231,199) * over255;

	float t = in_uv.y;

	vec3 color = mix(mix(c1, c2, t), mix(c2, c3, t), t);
    outColor = vec4(color, 1.0);
	
	//outColor = vec4(0.96, 0.89, 0.56, 1.0);
}
