#version 460
#extension GL_EXT_scalar_block_layout: require
#extension GL_EXT_shader_explicit_arithmetic_types_int64: require

layout (set = 1, binding = 0, scalar) uniform MaterialParams {
    vec4 color;
} matParams;

layout (location = 0) out vec4 outColor;

void main() {
    outColor = matParams.color;
}
