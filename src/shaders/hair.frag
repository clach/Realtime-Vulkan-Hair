#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 outColor;

void main() {
	float over255 = 1.0 / 255.0;

	// green gradient color
	vec3 green1 = vec3(171, 146, 99) * over255;
	vec3 green2 = vec3(216,192,120) * over255;
	//vec3 green2 = vec3(242,225,174) * over255;
	vec3 green3 = vec3(242,231,199) * over255;

	float t = in_uv.y;

	vec3 color = mix(mix(green1, green2, t), mix(green2, green3, t), t);
	//color = mix(mix(green1, green2, in_uv.y), mix(green2, green3, in_uv.y), in_uv.y);

    outColor = vec4(color, 1.0);
    
	
	
	//outColor = vec4(0.96, 0.89, 0.56, 1.0);
}
