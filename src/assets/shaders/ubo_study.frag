#version 460
#extension GL_EXT_scalar_block_layout: require
#extension GL_EXT_shader_explicit_arithmetic_types_int64: require

struct VertexOutput {
    vec3 objectPosition;
    vec3 worldPosition;
    vec3 objectNormal;
    vec3 worldNormal;
    vec4 color;
    vec2 texCoord0;
};

struct VertexIds {
    int meshId;
    int spanId;
};

layout (location = 0) in VertexOutput v;
layout (location = 6) flat in VertexIds ids;

layout (set = 0, binding = 0, scalar) uniform perFrameData {
    mat4 viewFromModel;
    mat4 projectionFromView;
    vec3 camPos;
    uint64_t frameNo;
} perFrame;

layout (set = 1, binding = 0, scalar) uniform MaterialParams {
    int cnt1;
    int cnt2;
    float num1;
    float num2;
    vec2 dir1;
    vec2 dir2;
    vec3 pos1;
    vec3 pos2;
    vec4 col1;
    vec4 col2;
    mat3 rot1;
    mat3 rot2;
    mat4 xform1;
    mat4 xform2;
} matParams;

layout (location = 0) out vec4 outColor;

void main() {
    outColor = vec4(v.objectPosition.x, ids.meshId, matParams.num1, 1);
}
