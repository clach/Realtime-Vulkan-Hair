#version 450
#extension GL_ARB_separate_shader_objects : enable
#define NUM_CURVE_POINTS 10

layout(isolines) in;

layout(set = 0, binding = 0) uniform CameraBufferObject {
    mat4 view;
    mat4 proj;
} camera;

layout(location = 0) in vec4[][NUM_CURVE_POINTS] in_curvePoints;

layout(location = 0) out vec2 out_uv;


void main() {
    float u = gl_TessCoord.y;
    float v = gl_TessCoord.x;

	out_uv = vec2(u, v);

	// If 0 or 1 curve points, there is no curve
	if (NUM_CURVE_POINTS <= 1) {
		gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	// Get relevant curve points
	vec3 v0; // previous point before segment
	vec3 v1; // current segment's first point
	vec3 v2; // current segment's second point
	vec3 v3; // next point after segment

	
	// All segments have first and second points
	int segmentFirst = int(floor(v * (NUM_CURVE_POINTS - 1)));
	int segmentSecond = segmentFirst + 1;
	v1 = in_curvePoints[0][segmentFirst].xyz;
	v2 = in_curvePoints[0][segmentSecond].xyz;

	if (segmentFirst == 0) {
		// If first segment
		v0 = v1 + (v1 - v2);
	} else {
		v0 = in_curvePoints[0][segmentFirst - 1].xyz;
	}

	if (segmentSecond == NUM_CURVE_POINTS - 1) {
		// If last segment
		v3 = v2 + (v2 - v1);
	} else {
		v3 = in_curvePoints[0][segmentSecond + 1].xyz;
	}

	// Create bezier control points based on 4 surrounding curve points
	vec3 b0 = v1;
	vec3 b1 = v1 + ((1.0 / 3.0) * ((v2 - v0) / 2.0));
	vec3 b2 = v2 - ((1.0 / 3.0) * ((v3 - v1) / 2.0));;
	vec3 b3 = v2;

	// Interpolation value
	float t = v * (NUM_CURVE_POINTS - 1) - floor(v * (NUM_CURVE_POINTS - 1));

	// De Casteljau's interpolation for Bezier curve position
	vec3 b01 = mix(b0, b1, t);
	vec3 b11 = mix(b1, b2, t);
	vec3 b21 = mix(b2, b3, t);
		 		   
	vec3 b02 = mix(b01, b11, t);
	vec3 b12 = mix(b11, b21, t);
		 		   		
	vec3 c = mix(b02, b12, t);

	float width = 0.1;
	vec3 t1 = vec3(1.0, 0.0, 0.0);
	vec3 c0 = c - width * t1;
	vec3 c1 = c + width * t1;

	vec4 pos = vec4(mix(c0, c1, u), 1.0);


	gl_Position = camera.proj * camera.view * pos;
}
