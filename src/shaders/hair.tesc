#version 450
#extension GL_ARB_separate_shader_objects : enable
#define NUM_CURVE_POINTS 10


layout(vertices = 1) out;

// TODO: Declare tessellation control shader inputs and outputs
layout(location = 0) in vec4[][NUM_CURVE_POINTS] in_curvePoints;

layout(location = 0) out vec4[][NUM_CURVE_POINTS] out_curvePoints;

void main() {
	// Don't move the origin location of the patch
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

	// TODO: write any shader outputs

	for (int i = 0; i < NUM_CURVE_POINTS; i++) {
		out_curvePoints[gl_InvocationID][i] = in_curvePoints[gl_InvocationID][i];
	}

     //gl_TessLevelInner[0] = 2;
     //gl_TessLevelInner[1] = 8;
     gl_TessLevelOuter[0] = 3;
     gl_TessLevelOuter[1] = 32;
     //gl_TessLevelOuter[2] = 8;
     //gl_TessLevelOuter[3] = 8;
}
