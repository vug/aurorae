#version 460
#extension GL_EXT_scalar_block_layout: require
#extension GL_EXT_shader_explicit_arithmetic_types_int64: require

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec3 bitangent;
layout (location = 4) in vec4 color;
layout (location = 5) in vec2 texCoord0;
layout (location = 6) in vec2 texCoord1;
layout (location = 7) in vec2 texCoord2;
layout (location = 8) in vec3 custom0;
layout (location = 9) in vec4 custom1;

layout (push_constant) uniform PushConstants {
    mat4 modelFromObject;
    mat4 transposeInverseTransform;
    int meshId;
    int spanId;
} pc;

layout (set = 0, binding = 0, scalar) uniform perFrameData {
    mat4 viewFromModel;
    mat4 projectionFromView;
    vec3 camPos;
    uint64_t frameNo;
} perFrame;

struct VertexOutput {
    vec3 objectPosition;
    vec3 worldPosition;
    vec3 objectNormal;
    vec3 worldNormal;
    vec4 color;
    vec2 texCoord0;
};

VertexOutput fillVertexOutput(mat4 worldFromObject, mat4 transposeInverseTransform, vec3 position, vec3 normal, vec2 texCoord0,
                              vec4 color) {
    VertexOutput v;
    v.objectPosition = position;
    // worldFromObject is affine, i.e. bottom row is [0, 0, 0, 1]
    v.worldPosition = vec3(worldFromObject * vec4(position, 1));
    // worldFromObject is not affine (e.g. contains some perspective/projection)
    // // const vec4 worldPos4 = worldFromObject * vec4(position, 1);
    // // v.worldPosition = vec3(worldPos4) / worldPos4.w;
    v.objectNormal = normal;
    v.worldNormal = normalize(vec3(transposeInverseTransform * vec4(normal, 0)));
    v.texCoord0 = texCoord0;
    v.color = color;
    return v;
}

struct VertexIds {
    int meshId;
    int spanId;
};

VertexIds fillVertexIds(int meshId, int spanId) {
    VertexIds v;
    v.meshId = meshId;
    v.spanId = spanId;
    return v;
}


layout (location = 0) out VertexOutput vertexOutput;
layout (location = 6) flat out VertexIds vertexIds;


void main() {
    vertexOutput = fillVertexOutput(
        pc.modelFromObject,
        pc.transposeInverseTransform,
        position,
        normal,
        texCoord0,
        color
    );

    vertexIds = fillVertexIds(pc.meshId,
                              pc.spanId);

    gl_Position = perFrame.projectionFromView * perFrame.viewFromModel * pc.modelFromObject * vec4(position, 1.0);
}