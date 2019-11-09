#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(quads, equal_spacing, ccw) in;

void main() {
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;
}
