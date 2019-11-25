#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragLightVec;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 color = vec3(texture(texSampler, fragTexCoord));
		
	// lambert shading
	float diffuseTerm = clamp(dot(normalize(fragNormal), normalize(fragLightVec)), 0, 1);
	float lightIntensity = diffuseTerm + 0.2; // add some ambient lighting

    outColor = vec4(color * lightIntensity, 1.0);
}
