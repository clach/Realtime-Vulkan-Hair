#version 450
#extension GL_ARB_separate_shader_objects : enable
#define NUM_CURVE_POINTS 10

layout(set = 1, binding = 0) uniform ModelBufferObject {
    mat4 model;
};

// hardcode for now
// TODO: declare vertex shader inputs/outputs
layout(location = 0) in vec4 in_curvePoints[NUM_CURVE_POINTS];
layout(location = 0) out vec4 out_curvePoints[NUM_CURVE_POINTS];

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	for (int i = 0; i < NUM_CURVE_POINTS; i++) {
		out_curvePoints[i] = model * in_curvePoints[i];
	}

	gl_Position = in_curvePoints[0];
}
