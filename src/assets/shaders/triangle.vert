#version 460

// Hardcoded vertex positions in Normalized Device Coordinates (NDC)
// and colors (RGB)
vec2 positions[3] = vec2[](
vec2(0.0, -0.5), // Bottom center
vec2(0.5, 0.5), // Top right
vec2(-0.5, 0.5)  // Top left
);

vec3 colors[3] = vec3[](
vec3(1.0, 0.0, 0.0), // Red
vec3(0.0, 1.0, 0.0), // Green
vec3(0.0, 0.0, 1.0)  // Blue
);

layout (location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
