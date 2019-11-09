#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform ModelBufferObject {
    mat4 model;
};

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	gl_Position =vec4(0.0);
}
