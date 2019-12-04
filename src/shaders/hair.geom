#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

layout(set = 0, binding = 0) uniform CameraBufferObject {
    mat4 view;
    mat4 proj;
} camera;

layout(location = 0) in vec2 in_uv[];
layout(location = 1) in vec3 in_u[];
layout(location = 2) in vec3 in_v[];
layout(location = 3) in vec3 in_w[];
layout(location = 4) in vec3 in_viewDir[];
layout(location = 5) in vec3 in_lightDir[];

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_u;
layout(location = 2) out vec3 out_v;
layout(location = 3) out vec3 out_w;
layout(location = 4) out vec3 out_viewDir;
layout(location = 5) out vec3 out_lightDir;

void main() {
	// TODO: play around with this for LOD/AA purposes
	float distToCamera = length(in_viewDir[0]);
	float width = 0.005f * (log(0.1 * distToCamera) + 5.f);

	width *= 0.5;
	
	gl_Position = gl_in[0].gl_Position + vec4(-width, 0.0, 0.0, 0.0);
	out_uv = in_uv[0];
	out_u = in_u[0];
	out_v = in_v[0];
	out_w = in_w[0];
	out_viewDir = normalize(in_viewDir[0]);
	out_lightDir = in_lightDir[0];
	EmitVertex();

	gl_Position = gl_in[0].gl_Position + vec4(width, 0.0, 0.0, 0.0);
	out_uv = in_uv[0];
	out_u = in_u[0];
	out_v = in_v[0];
	out_w = in_w[0];
	out_viewDir = normalize(in_viewDir[0]);
	out_lightDir = in_lightDir[0];
	EmitVertex();

	gl_Position = gl_in[1].gl_Position + vec4(-width, 0.0, 0.0, 0.0);
	out_uv = in_uv[1];
	out_u = in_u[1];
	out_v = in_v[1];
	out_w = in_w[1];
	out_viewDir = normalize(in_viewDir[1]);
	out_lightDir = in_lightDir[1];
	EmitVertex();

	gl_Position = gl_in[1].gl_Position + vec4(width, 0.0, 0.0, 0.0);
	out_uv = in_uv[1];
	out_u = in_u[1];
	out_v = in_v[1];
	out_w = in_w[1];
	out_viewDir = normalize(in_viewDir[1]);
	out_lightDir = in_lightDir[1];
	EmitVertex();

	EndPrimitive();	
}