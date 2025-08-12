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
    const int vizMode = 3;
    switch (vizMode) {
        case 0: {
                    outColor = vec4(v.objectPosition, 1.0);
                } break;
        case 1: {
                    outColor = vec4(v.worldPosition, 1.0);
                } break;
        case 2: {
                    outColor = vec4(v.objectNormal * 0.5 + 0.5, 1.0);
                } break;
        case 3: {
                    outColor = vec4(v.worldNormal * 0.5 + 0.5, 1.0);
                } break;
        case 4: {
                    outColor = vec4(v.color.rgb, 1);
                } break;
        case 5: {
                    outColor = vec4(v.texCoord0, 0.0, 1.0);
                } break;
        case 6: {
                    float meshColor = float(ids.meshId) * 0.618033988749;
                    meshColor = fract(meshColor);
                    outColor = vec4(vec3(meshColor), 1.0);
                } break;
        case 7: {
                    float spanColor = float(ids.spanId) * 3.14159265358979;
                    spanColor = fract(spanColor);
                    outColor = vec4(vec3(spanColor), 1.0);
                } break;
        case 8: {
                    float meshColor = fract(float(ids.meshId) * 0.618033988749);
                    float spanColor = fract(float(ids.spanId) * 0.618033988749);
                    outColor = vec4(meshColor, spanColor, 0.0, 1.0);
                } break;
        default: {
                    outColor = vec4(1.0, 0.0, 1.0, 1.0);
                } break;

    }
}
