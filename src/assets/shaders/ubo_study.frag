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

struct PointLight {
    vec3 position;
    vec4 color;
    float intensity;
};

struct Struct2 {
    int var1;
};

struct Struct1 {
    int var1;
    int var2;
    Struct2[2] var3;
};

layout (set = 1, binding = 0, scalar) uniform MaterialParams {
    vec3[5] positions;
    int pointLightCnt;
    PointLight[10] pointLights;
    Struct1 s1;
} matParams;

layout (location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1, 0, 0, 1);
}
