#version 460

layout (location = 0) in vec3 vObjectNormal;
layout (location = 1) in vec3 vWorldNormal;
layout (location = 2) in vec4 vColor;
layout (location = 3) in vec2 vTexCoord0;

layout (location = 0) out vec4 outColor;

void main() {
    //    outColor = vec4(vColor, 1.0);
    //    outColor = vec4(vTexCoord0, 0, 1.0);
    outColor = vec4(vWorldNormal, 1.0);
}
