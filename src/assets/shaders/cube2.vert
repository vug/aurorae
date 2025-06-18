#version 450
#extension GL_EXT_scalar_block_layout: require

// The vertices are hard-coded in the draw command, so we use gl_VertexIndex
// to generate the cube vertices on the fly.

layout(push_constant) uniform PushConstants {
    mat4 modelFromObject;
} pc;


// --- Uniform Buffer for Scene Data ---
// "scalar" layout makes this struct match C++ memory layout exactly.
// "binding = 0" corresponds to the binding in the descriptor set layout.
layout (set = 0, binding = 0, scalar) uniform perFrameData {
    mat4 viewFromModel;
    mat4 projectionFromView;
} perFrame;

// Simple cube vertices and colors generated from the vertex index
const vec3 positions[8] = vec3[](
vec3(-1.0, -1.0, 1.0),
vec3(1.0, -1.0, 1.0),
vec3(1.0, 1.0, 1.0),
vec3(-1.0, 1.0, 1.0),
vec3(-1.0, -1.0, -1.0),
vec3(1.0, -1.0, -1.0),
vec3(1.0, 1.0, -1.0),
vec3(-1.0, 1.0, -1.0)
);

const int indices[36] = int[](
0, 1, 2, 2, 3, 0, // Front face
1, 5, 6, 6, 2, 1, // Right face
7, 6, 5, 5, 4, 7, // Back face
4, 0, 3, 3, 7, 4, // Left face
3, 2, 6, 6, 7, 3, // Top face
4, 5, 1, 1, 0, 4  // Bottom face
);

// Assign a unique color to each vertex
const vec3 colors[8] = vec3[](
vec3(1.0, 0.0, 0.0), // Red
vec3(0.0, 1.0, 0.0), // Green
vec3(0.0, 0.0, 1.0), // Blue
vec3(1.0, 1.0, 0.0), // Yellow
vec3(1.0, 0.0, 1.0), // Magenta
vec3(0.0, 1.0, 1.0), // Cyan
vec3(1.0, 1.0, 1.0), // White
vec3(0.0, 0.0, 0.0)  // Black
);

layout (location = 0) out vec3 vColor;

void main() {
    // Get the vertex position and color for the current index
    int vertex_idx = indices[gl_VertexIndex];
    vec3 position = positions[vertex_idx];
    vColor = colors[vertex_idx];

    // Note: We don't have a model matrix yet, just view and projection.
    gl_Position = perFrame.projectionFromView * perFrame.viewFromModel * pc.modelFromObject * vec4(position, 1.0);
}