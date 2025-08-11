#version 460
#extension GL_EXT_scalar_block_layout: require

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
//layout (location = 0) in vec3 bitangent;
layout (location = 3) in vec4 color;
layout (location = 4) in vec2 texCoord0;
layout (location = 5) in vec2 texCoord1;
layout (location = 6) in vec2 texCoord2;
layout (location = 7) in vec3 custom0;
layout (location = 8) in vec4 custom1;

layout (push_constant) uniform PushConstants {
    mat4 modelFromObject;
} pc;

layout (set = 0, binding = 0, scalar) uniform perFrameData {
    mat4 viewFromModel;
    mat4 projectionFromView;
} perFrame;

layout (location = 0) out vec4 vColor;

void main() {
    vColor = color;
    gl_Position = perFrame.projectionFromView * perFrame.viewFromModel * pc.modelFromObject * vec4(position, 1.0);
}