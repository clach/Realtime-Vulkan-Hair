#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

layout(set = 0, binding = 0) uniform CameraBufferObject {
    mat4 view;
    mat4 proj;
} camera;

layout(set = 2, binding = 0) uniform ShadowCameraBufferObject {
    mat4 view;
    mat4 proj;
} shadowCamera;

layout(location = 0) in vec2 in_uv[];
layout(location = 1) in vec3 in_u[];
layout(location = 2) in vec3 in_v[];
layout(location = 3) in vec3 in_w[];
layout(location = 4) in vec3 in_viewDir[];
layout(location = 5) in vec3 in_lightDir[];
layout(location = 6) in float in_strandWidth[];
//layout(location = 7) in vec3 in_fragPos[];
//layout(location = 7) in vec4 in_fragPosLightSpace[];

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_u;
layout(location = 2) out vec3 out_v;
layout(location = 3) out vec3 out_w;
layout(location = 4) out vec3 out_viewDir;
layout(location = 5) out vec3 out_lightDir;
layout(location = 6) out vec4 out_fragPosLightSpace;

// https://thebookofshaders.com/10/
float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

void main() {
	// TODO: play around with this for LOD/AA purposes
	float distToCamera = length(in_viewDir[0]);

	float u = in_uv[0].x; // u will be the same for entire strand
	float rand = abs(random(vec2(3.242551 * u, u * u))) + 0.5; 
	float scale = rand * 0.075 * (distToCamera);
	float width1 = scale * in_strandWidth[0];
	float width2 = scale * in_strandWidth[1];

	vec4 pos1 = camera.proj * camera.view * gl_in[0].gl_Position;
	//gl_Position = gl_in[0].gl_Position + vec4(-width1, 0.0, 0.0, 0.0);
	vec4 newPos1_1 = pos1 + vec4(-width1, 0.0, 0.0, 0.0);
	gl_Position = newPos1_1;
	out_uv = in_uv[0];
	out_u = in_u[0];
	out_v = in_v[0];
	out_w = in_w[0];
	out_viewDir = normalize(in_viewDir[0]);
	out_lightDir = in_lightDir[0];
	out_fragPosLightSpace = shadowCamera.proj * shadowCamera.view * inverse(camera.view) * inverse(camera.proj) * newPos1_1;
	EmitVertex();

	//gl_Position = gl_in[0].gl_Position + vec4(width1, 0.0, 0.0, 0.0);
	//gl_Position = pos1 + vec4(width1, 0.0, 0.0, 0.0);
	vec4 newPos1_2 = pos1 + vec4(width1, 0.0, 0.0, 0.0);
	gl_Position = newPos1_2;
	out_uv = in_uv[0];
	out_u = in_u[0];
	out_v = in_v[0];
	out_w = in_w[0];
	out_viewDir = normalize(in_viewDir[0]);
	out_lightDir = in_lightDir[0];
	//out_fragPosLightSpace = in_fragPosLightSpace[0];
	out_fragPosLightSpace = shadowCamera.proj * shadowCamera.view * inverse(camera.view) * inverse(camera.proj) * newPos1_2;
	EmitVertex();


	vec4 pos2 = camera.proj * camera.view * gl_in[1].gl_Position;
	//gl_Position = gl_in[1].gl_Position + vec4(-width2, 0.0, 0.0, 0.0);
	//gl_Position = pos2 + vec4(-width2, 0.0, 0.0, 0.0);
	vec4 newPos2_1 = pos2 + vec4(-width1, 0.0, 0.0, 0.0);
	gl_Position = newPos2_1;
	out_uv = in_uv[1];
	out_u = in_u[1];
	out_v = in_v[1];
	out_w = in_w[1];
	out_viewDir = normalize(in_viewDir[1]);
	out_lightDir = in_lightDir[1];
	//out_fragPosLightSpace = in_fragPosLightSpace[1];
	out_fragPosLightSpace = shadowCamera.proj * shadowCamera.view * inverse(camera.view) * inverse(camera.proj) * newPos2_1;
	EmitVertex();

	//gl_Position = gl_in[1].gl_Position + vec4(width2, 0.0, 0.0, 0.0);
	//gl_Position = pos2 + vec4(width2, 0.0, 0.0, 0.0);
	vec4 newPos2_2 = pos2 + vec4(width1, 0.0, 0.0, 0.0);
	gl_Position = newPos2_2;
	out_uv = in_uv[1];
	out_u = in_u[1];
	out_v = in_v[1];
	out_w = in_w[1];
	out_viewDir = normalize(in_viewDir[1]);
	out_lightDir = in_lightDir[1];
	//out_fragPosLightSpace = in_fragPosLightSpace[1];
	out_fragPosLightSpace = shadowCamera.proj * shadowCamera.view * inverse(camera.view) * inverse(camera.proj) * newPos2_2;
	EmitVertex();

	EndPrimitive();	
}