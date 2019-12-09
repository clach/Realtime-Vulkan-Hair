#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform CameraBufferObject {
    mat4 view;
	mat4 proj;
} camera;

layout(set = 2, binding = 0) uniform ModelBufferObject {
    mat4 model;
	mat4 invTransModel;
};

layout(set = 3, binding = 0) uniform ShadowCameraBufferObject {
    mat4 view;
	mat4 proj;
} shadowCamera;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragLightVec;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	vec3 modelPos = vec3(model * vec4(inPosition, 1.0));

	mat4 invLightView = inverse(shadowCamera.view); // TODO: compute ahead of time?
	vec3 lightPos = vec3(invLightView[3][0], invLightView[3][1], invLightView[3][2]);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
	fragNormal = vec3(invTransModel * vec4(inNormal, 0.0));
	fragLightVec = lightPos - modelPos;

    gl_Position = camera.proj * camera.view * vec4(modelPos, 1.0);
}
