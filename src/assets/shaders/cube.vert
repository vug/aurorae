#version 460

// Hardcoded vertex positions for a cube (36 vertices for 12 triangles)
// Each face is two triangles.
// We'll define them counter-clockwise for default front-face culling.
vec3 positions[36] = vec3[](
// Front face
vec3(-0.5, -0.5, 0.5), vec3(0.5, -0.5, 0.5), vec3(0.5, 0.5, 0.5),
vec3(-0.5, -0.5, 0.5), vec3(0.5, 0.5, 0.5), vec3(-0.5, 0.5, 0.5),
// Back face
vec3(-0.5, -0.5, -0.5), vec3(-0.5, 0.5, -0.5), vec3(0.5, 0.5, -0.5),
vec3(-0.5, -0.5, -0.5), vec3(0.5, 0.5, -0.5), vec3(0.5, -0.5, -0.5),
// Left face
vec3(-0.5, -0.5, -0.5), vec3(-0.5, -0.5, 0.5), vec3(-0.5, 0.5, 0.5),
vec3(-0.5, -0.5, -0.5), vec3(-0.5, 0.5, 0.5), vec3(-0.5, 0.5, -0.5),
// Right face
vec3(0.5, -0.5, -0.5), vec3(0.5, 0.5, 0.5), vec3(0.5, -0.5, 0.5),
vec3(0.5, -0.5, -0.5), vec3(0.5, 0.5, -0.5), vec3(0.5, 0.5, 0.5),
// Top face
vec3(-0.5, 0.5, 0.5), vec3(0.5, 0.5, 0.5), vec3(0.5, 0.5, -0.5),
vec3(-0.5, 0.5, 0.5), vec3(0.5, 0.5, -0.5), vec3(-0.5, 0.5, -0.5),
// Bottom face
vec3(-0.5, -0.5, 0.5), vec3(0.5, -0.5, -0.5), vec3(0.5, -0.5, 0.5),
vec3(-0.5, -0.5, 0.5), vec3(-0.5, -0.5, -0.5), vec3(0.5, -0.5, -0.5)
);

// Simple colors per face (6 faces, 2 triangles per face, 3 vertices per triangle)
vec3 colors[36] = vec3[](
// Front face (Red)
vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0),
vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0),
// Back face (Green)
vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0),
vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0),
// Left face (Blue)
vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0),
vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0),
// Right face (Yellow)
vec3(1.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0),
vec3(1.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0),
// Top face (Cyan)
vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 1.0),
vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 1.0),
// Bottom face (Magenta)
vec3(1.0, 0.0, 1.0), vec3(1.0, 0.0, 1.0), vec3(1.0, 0.0, 1.0),
vec3(1.0, 0.0, 1.0), vec3(1.0, 0.0, 1.0), vec3(1.0, 0.0, 1.0)
);

layout (location = 0) out vec3 fragColor;

void main() {
    // Basic perspective projection and view transform (looking at origin)
    // You'll want to replace this with proper MVP matrices later.
    mat4 projection = mat4(
    1.0, 0.0, 0.0, 0.0,
    0.0, -1.0, 0.0, 0.0, // Flip Y for Vulkan
    0.0, 0.0, 0.5, 0.5, // Depth mapping (0 to 1)
    0.0, 0.0, 0.0, 1.0
    );
    // Simple view matrix: camera at (0, 0, -2), looking towards origin
    mat4 view = mat4(
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 2.0, 1.0 // Translate camera back
    );
    // Simple model matrix: scale the cube down a bit
    mat4 model = mat4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 0.5, 0.0,
    0.0, 0.0, 0.0, 1.0
    );

    gl_Position = projection * view * model * vec4(positions[gl_VertexIndex], 1.0);
    fragColor = colors[gl_VertexIndex];
}

