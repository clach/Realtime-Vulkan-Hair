#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(vertices = 1) out;

// TODO: Declare tessellation control shader inputs and outputs
layout(location = 0) in vec4[][3] in_controlPoints;

layout(location = 0) out vec4[][3] out_controlPoints;

void main() {
	// Don't move the origin location of the patch
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

	// TODO: write any shader outputs

	for (int i = 0; i < 3; i++) {
		out_controlPoints[gl_InvocationID][i] = in_controlPoints[gl_InvocationID][i];
	}

     //gl_TessLevelInner[0] = 2;
     //gl_TessLevelInner[1] = 8;
     gl_TessLevelOuter[0] = 4;
     gl_TessLevelOuter[1] = 32;
     //gl_TessLevelOuter[2] = 8;
     //gl_TessLevelOuter[3] = 8;
}
