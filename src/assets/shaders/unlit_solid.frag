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
    uint64_t frameNo;
} perFrame;

layout (set = 1, binding = 0, scalar) uniform MaterialParams {
    vec4 color;
} matParams;

layout (location = 0) out vec4 outColor;

void main() {
    outColor = matParams.color;
}
