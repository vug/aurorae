#version 460

layout (location = 0) in vec4 vColor;
layout (location = 0) out vec4 outColor;

void main() {
    outColor = vec4(vColor.rgb, 0.5);
}
