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

float shadowCalculation(vec4 fragPosLightSpace) {
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords.xy = projCoords.xy * 0.5 + 0.5;
	float closestDepth = texture(depthSampler, projCoords.xy).r;
	float currentDepth = projCoords.z;
	float shadow = currentDepth > closestDepth ? 1.0 : 0.0;
	//return shadow;
	return closestDepth;
}

void main() {
	const float over255 = 1.0 / 255.0;


	vec3 projCoords = in_fragPosLightSpace.xyz / in_fragPosLightSpace.w;
	projCoords.xy = projCoords.xy * 0.5 + 0.5;
	float z0 = texture(depthSampler, projCoords.xy).r;
	float currentDepth = projCoords.z;
	//float shadow = currentDepth > closestDepth ? 1.0 : 0.0;

	const float d = 0.3;

	float layer1  = 0.f;
	float layer2  = 0.f;
	float layer3  = 0.f;

	float val = 0.3f;

	if (currentDepth >= z0 && currentDepth < z0 + d) {
		layer1 += val;
		layer2 += val;
		layer3 += val;
	} else if (currentDepth >= z0 + d && currentDepth < z0 + 2.f * d) {
		layer2 += val;
		layer3 += val;
	} else if (currentDepth >= z0 + 2.f * d) { 
		layer3 += val;
	}

	//float shadow = shadowCalculation(in_fragPosLightSpace);
	//vec3 colorTest = vec3(shadow);

	outColor = vec4(layer1, layer2, layer3, z0); 
	//outColor = vec4(layer1, 0, 0, 1.f); 
	//outColor = vec4(0, layer2, 0, 1.f); 
}
