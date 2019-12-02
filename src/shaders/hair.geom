#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location = 0) in vec2 in_uv[];
layout(location = 0) out vec2 out_uv;


void main() {
	float width = 0.01;
	
	gl_Position = gl_in[0].gl_Position + vec4(-width, 0.0, 0.0, 0.0);
	out_uv = in_uv[0];
	EmitVertex();

	gl_Position = gl_in[0].gl_Position + vec4(width, 0.0, 0.0, 0.0);
	out_uv = in_uv[0];
	EmitVertex();

	gl_Position = gl_in[1].gl_Position + vec4(-width, 0.0, 0.0, 0.0);
	out_uv = in_uv[1];
	EmitVertex();

	gl_Position = gl_in[1].gl_Position + vec4(width, 0.0, 0.0, 0.0);
	out_uv = in_uv[1];
	EmitVertex();

	EndPrimitive();	
}