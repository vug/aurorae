#version 460

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

layout (location = 0) out vec4 outColor;

void main() {
    //    outColor = vec4(vertexOutput.texCoord0, 0, 1.0);
    outColor = vec4(v.worldNormal, 1.0);
    // outColor = vec4(ids.meshId * 0.5 + 0.25, 0, 0, 1);
}
