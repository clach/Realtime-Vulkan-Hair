#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform ModelBufferObject {
    mat4 model;
};

// hardcode for now
// TODO: declare vertex shader inputs/outputs
layout(location = 0) in vec4 in_controlPoints[3];
layout(location = 0) out vec4 out_controlPoints[3];

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	for (int i = 0; i < 3; i++) {
		out_controlPoints[i] = model * in_controlPoints[i];
	}

	gl_Position = vec4(in_controlPoints[0].xyz, 1.0);
}
