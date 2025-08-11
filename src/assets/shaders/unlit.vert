#version 460
#extension GL_EXT_scalar_block_layout: require

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
} perFrame;


layout (location = 0) out vec3 vObjectNormal;
layout (location = 1) out vec3 vWorldNormal;
layout (location = 2) out vec4 vColor;
layout (location = 3) out vec2 vTexCoord0;

void main() {
    // TODO(vug): calculate on CPU
    const mat4 normalMatrix = transpose(inverse(pc.modelFromObject));
    vObjectNormal = normal;
    vWorldNormal = normalize(vec3(normalMatrix * vec4(normal, 1)));
    vColor = color;
    vTexCoord0 = texCoord0;
    gl_Position = perFrame.projectionFromView * perFrame.viewFromModel * pc.modelFromObject * vec4(position, 1.0);
}