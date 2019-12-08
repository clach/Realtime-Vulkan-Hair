#version 450
#extension GL_ARB_separate_shader_objects : enable
#define PI 3.141592653
#define NUM_CURVE_POINTS 10

layout(isolines) in;

layout(set = 0, binding = 0) uniform CameraBufferObject {
    mat4 view;
    mat4 proj;
} camera;

layout(set = 2, binding = 0) uniform ShadowCameraBufferObject {
    mat4 view;
    mat4 proj;
} shadowCamera;

layout(location = 0) in vec4[][NUM_CURVE_POINTS] in_curvePoints;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_u;
layout(location = 2) out vec3 out_v;
layout(location = 3) out vec3 out_w;
layout(location = 4) out vec3 out_viewDir;
layout(location = 5) out vec3 out_lightDir;
layout(location = 6) out float out_strandWidth;
//layout(location = 7) out vec4 out_fragPosLightSpace;

// https://thebookofshaders.com/10/
float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

vec3 func(float u, float v) {
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
	return c;
}

vec3 stupidFunc(float u, float v) {
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
		 	
	return vec3(b12 - b02);
}

// Hughes-Moller method for computing orthonormal vector
void hughesMoellerMethod(vec3 n, out vec3 b1, out vec3 b2) {
	if (abs(n.x) > abs(n.z)) {
		b1 = vec3(-n.y, n.x, 0.f);
	} else {
		b1 = vec3(0.f, -n.z, n.y);
	}
	b1 = normalize(b1);
	b2 = normalize(cross(n, b1));
}


void frisvadONB(vec3 n, out vec3 b1, out vec3 b2)
{
	if (n.z < -0.9999999f) {
		b1 = vec3( 0.0f, -1.0f, 0.0f);
		b2 = vec3(-1.0f, 0.0f, 0.0f);
		return;
	 }
	 const float a = 1.0f / (1.0f + n.z);
	 const float b = -n.x * n.y * a;
	 b1 = normalize(vec3(1.0f - n.x * n.x * a, b, -n.x));
	 b2 = normalize(vec3(b, 1.0f - n.y * n.y * a, -n.y));
}

void main() {
    float u = gl_TessCoord.y;
    float v = gl_TessCoord.x;

	out_uv = vec2(u, v);

	// If 0 or 1 curve points, there is no curve
	if (NUM_CURVE_POINTS <= 1) {
		gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}


	float sd = 1.0;
	float division = 1.0 / float(NUM_CURVE_POINTS - 1);

	vec3 currRoot = in_curvePoints[0][0].xyz;

	float randomChoice = random(vec2(u, currRoot.x)) * random(vec2(currRoot.y, currRoot.z));

	if (randomChoice > 0.5) {
		float maxLength = 2.0;
		int maxCV = int(floor(maxLength * float(NUM_CURVE_POINTS - 1))) + 2;
		if (randomChoice > 0.9) {
			sd = 2.5f * exp(-pow(v - 0.25, 2.0) / (2.0 * pow(0.2, 2.0)));
		} else if (randomChoice > 0.8) {
			sd = 6.5f * pow(v, 10.0);
		} else if (randomChoice > 0.7) {
			sd = 4.8f * exp(-pow(v - 0.7, 2.0) / (2.0 * pow(0.2, 2.0)));
		} else if (randomChoice > 0.6) {
			sd = 6.f * pow(v, 1.3);
		} else {
			sd = 2.4f * exp(-pow(v - 0.8, 2.0) / (2.0 * pow(0.2, 2.0)));
		}
	}

	if (v == 0.0) {
		sd = 1.0;
	}


	// Get relevant curve points
	//vec3 v0; // previous point before segment
	//vec3 v1; // current segment's first point
	//vec3 v2; // current segment's second point
	//vec3 v3; // next point after segment

	// All segments have first and second points
	int segmentFirst = int(floor(v * (NUM_CURVE_POINTS - 1)));
	int segmentSecond = segmentFirst + 1;

	float w1 = abs(random(vec2(3.24242 * u, u)));
	float w2 = abs(random(vec2(u * u, u * u)));
	if (w1 + w2 >= 1) {
		w1 = 1.f - w1;
		w2 = 1.f - w2;
	}
	float w3 = abs(1.f - w1 - w2);

	vec3 v1_1 = in_curvePoints[0][segmentFirst].xyz;
	vec3 v1_2 = in_curvePoints[1][segmentFirst].xyz;
	vec3 v1_3 = in_curvePoints[2][segmentFirst].xyz;
	vec3 v1 = v1_1 * w1 + v1_2 * w2 + v1_3 * w3;
							   
	vec3 v2_1 = in_curvePoints[0][segmentSecond].xyz;
	vec3 v2_2 = in_curvePoints[1][segmentSecond].xyz;
	vec3 v2_3 = in_curvePoints[2][segmentSecond].xyz;
	vec3 v2 = v2_1 * w1 + v2_2 * w2 + v2_3 * w3;

	vec3 v0;
	if (segmentFirst == 0) {
		// If first segment
		v0 = v1 + (v1 - v2);
	} else {
		vec3 v0_1 = in_curvePoints[0][segmentFirst - 1].xyz;
		vec3 v0_2 = in_curvePoints[1][segmentFirst - 1].xyz;
		vec3 v0_3 = in_curvePoints[2][segmentFirst - 1].xyz;
		v0 = v0_1 * w1 + v0_2 * w2 + v0_3 * w3;
	}

	vec3 v3;
	if (segmentSecond == NUM_CURVE_POINTS - 1) {
		// If last segment
		v3 = v2 + (v2 - v1);
	} else {
		vec3 v3_1 = in_curvePoints[0][segmentSecond + 1].xyz;
		vec3 v3_2 = in_curvePoints[1][segmentSecond + 1].xyz;
		vec3 v3_3 = in_curvePoints[2][segmentSecond + 1].xyz;
		v3 = v3_1 * w1 + v3_2 * w2 + v3_3 * w3;
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

	// multi-strand tessellation
	vec3 pos = c;



	// let width be a function of v with some randomness
	// TODO: play with randomness, it's the same for each strand rn
	float rand2 = abs(random(vec2(u, u * u)));
	float dev = 0.f;
	if (rand2 > 0.95) {
		//dev = 0.9;
	} else if (rand2 > 0.8) {
		//dev = 0.5;
	} else {
		//dev = 0.1;
	}
	float width = (random(vec2(3.534 * u, u * 37.19817)) + 0.1) * 0.2 * exp(-pow(v - 0.5, 2.0) / (2.0 * pow(0.2, 2.0)));
	const float clumpRadius = 0.5f;
	width = clumpRadius * mix(0.3, 0.1, v) * (rand2 + 0.5);// add this for "curly" hair + random(vec2(u, v)) * 0.1;
	float uRad = 2.f * PI * u; // remap u to (0, 2pi)
	vec3 dir = normalize(vec3(cos(uRad), 0.f, sin(uRad)));

	float u1 = abs(random(vec2(u, u)));
	float u2 = abs(random(vec2(u, u * u)));
	float x1 = sqrt(-2.f * log(u1)) * cos(2.f * PI * u2);
	float x2 = sqrt(-2.f * log(u2)) * cos(2.f * PI * u1);


	// caculate orthonormal basis for shading
	//vec3 tangent = normalize(stupidFunc(u, v));
	vec3 tangent1 = normalize(in_curvePoints[0][segmentSecond].xyz - in_curvePoints[0][segmentFirst].xyz);
	vec3 tangent2 = normalize(in_curvePoints[1][segmentSecond].xyz - in_curvePoints[1][segmentFirst].xyz);
	vec3 tangent3 = normalize(in_curvePoints[2][segmentSecond].xyz - in_curvePoints[2][segmentFirst].xyz);
	vec3 b_1; 
	vec3 b_2;
	//hughesMoellerMethod(tangent, b_1, b_2);
	frisvadONB(tangent1, b_1, b_2);

//	tangent1 *= sd;
//	b_1 *= sd;
//	b_2 *= sd;
	dir *= sd;


	out_u = tangent1;
	out_v = b_1;
	out_w = b_2;



	u1 = (u1 * 2.f) + 1.f;
	u2 = (u2 * 2.f) + 1.f;
	//vec3 newDir = normalize(x1 * out_v + x2 * out_w);

	// single stranding tessellation
	vec3 singleStrandPos = func(u, v) + width * dir;



	float dist1 = distance(in_curvePoints[0][segmentFirst].xyz, in_curvePoints[1][segmentFirst].xyz);
	float dist2 = distance(in_curvePoints[1][segmentFirst].xyz, in_curvePoints[2][segmentFirst].xyz);
	float dist3 = distance(in_curvePoints[2][segmentFirst].xyz, in_curvePoints[0][segmentFirst].xyz);
	float angle1 = acos(dot(tangent1, tangent2) / (length(tangent1) * length(tangent2)));
	float angle2 = acos(dot(tangent2, tangent3) / (length(tangent2) * length(tangent3)));
	float angle3 = acos(dot(tangent1, tangent3) / (length(tangent1) * length(tangent3)));
	const float maxDist = 0.5f;
	if (dist1 > maxDist || dist2 > maxDist || dist3 > maxDist) {
		//pos = singleStrandPos;
	}

	pos = singleStrandPos;

	mat4 invLightView = inverse(shadowCamera.view); // TODO: compute ahead of time?
	vec3 lightPos = vec3(invLightView[3][0], invLightView[3][1], invLightView[3][2]);

	//vec3 lightPos = vec3(3.f, -1.f, 1.f);
	out_lightDir = normalize(lightPos - pos);
	mat4 invView = inverse(camera.view); // TODO: compute ahead of time?
	vec3 cameraPos = vec3(invView[3][0], invView[3][1], invView[3][2]);
	out_viewDir = cameraPos - pos;

	const float rootWidth = 0.05;
	const float tipWidth = 0.01;
	out_strandWidth = mix(rootWidth, tipWidth, v);


	//out_fragPosLightSpace = shadowCamera.proj * shadowCamera.view * vec4(pos, 1.0);



	//gl_Position = camera.proj * camera.view * vec4(pos, 1.0);
	gl_Position = vec4(pos, 1.0);
}

