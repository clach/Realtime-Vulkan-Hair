#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform CameraBufferObject {
    mat4 view;
	mat4 proj;
} camera;

layout(set = 1, binding = 0) uniform ModelBufferObject {
    mat4 model;
	mat4 invTransModel;
};

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
	vec3 lightPos = vec3(5.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
	fragNormal = vec3(invTransModel * vec4(inNormal, 0.0));
	fragLightVec = lightPos - modelPos;

    gl_Position = camera.proj * camera.view * vec4(modelPos, 1.0);
}
