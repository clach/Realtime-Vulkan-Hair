#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(quads, equal_spacing, ccw) in;

layout(set = 0, binding = 0) uniform CameraBufferObject {
    mat4 view;
    mat4 proj;
} camera;

// TODO: Declare tessellation evaluation shader inputs and outputs
layout(location = 0) in vec4[][3] in_controlPoints;

void main() {
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;


	// hard coded for now
	// TODO: write smarter b-spline/bezier evaluation

	// gather input
	vec3 v0 = in_controlPoints[0][0].xyz;
	vec3 v1 = in_controlPoints[0][1].xyz;
	vec3 v2 = in_controlPoints[0][2].xyz;
	float width = 0.1f;
	float orientation = 0.5f * 2.f * 3.14159265f;

	// deCasteljau's to evaluate Bezier curve
	vec3 a = mix(v0, v1, v);
	vec3 b = mix(v1, v2, v);
	vec3 c = mix(a, b, v);

	//vec3 t1 = normalize(vec3(cos(orientation + PI), 0.0, sin(orientation + PI)));
	vec3 t1 = normalize(vec3(cos(orientation), 0.0, sin(orientation))); // pointing along width of blade

	vec3 c0 = c - width * t1;
	vec3 c1 = c + width * t1;

	// for triangle shape
	float t = u + 0.5 * v - u * v;
	vec4 pos = vec4(mix(c0, c1, t), 1.0);

	gl_Position = camera.proj * camera.view * pos;
}
