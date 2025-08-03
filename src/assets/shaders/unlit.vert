#version 460
#extension GL_EXT_scalar_block_layout: require

layout (location = 0) in vec3 position;
layout (location = 1) in vec4 color;

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