# Aurorae - Vulkan 3D Renderer Context Document

## Project Overview

**Aurorae** is an early-stage, personal 3D renderer built with **Vulkan 3/4** and **C++23**. It serves as a workshop for
studying and experimenting with various computer graphics algorithms and techniques.

- **Platform**: Windows (MSVC)
- **Language**: C++23 with heavy use of modern features (concepts, ranges, modules preparation)
- **Build System**: CMake with presets (Debug, DebugAsan, Release, RelWithDebInfo)
- **Location**: `C:\Users\Ugur\repos\aurorae\src`

## Project Structure

### Core Components

```
src/
├── Application.h/cpp        - Main application orchestrator
├── Renderer.h/cpp           - Core rendering engine (commands, frame management)
├── VulkanContext.h/cpp      - Vulkan instance/device/queue management
├── Window.h/cpp             - GLFW window wrapper
├── Swapchain.h/cpp          - Swapchain and presentation
├── Pipeline.h/cpp           - Graphics pipeline abstraction
├── AppContext.h/cpp         - Global context for asset/renderer access
├── asset/                   - Asset processing and management
│   ├── AssetManager.h/cpp   - Runtime asset loading and caching
│   ├── AssetProcessor.h/cpp - Offline asset compilation (shaders, meshes)
│   ├── AssetRegistry.h/cpp  - Asset database (processedAssets/registry.json)
│   ├── ShaderReflection.h   - SPIRV-Cross shader introspection
│   ├── ShaderStage.h        - Individual shader stage assets
│   ├── GraphicsProgram.h    - Shader program (vert+frag) assets
│   ├── Material.h           - Material asset definitions
│   └── Mesh.h               - Mesh asset definitions
├── render/                  - GPU-side render objects
│   ├── GraphicsProgram.h/cpp- Uploaded shader programs
│   ├── Material.h/cpp       - Uploaded materials with UBOs
│   └── Mesh.h/cpp           - Uploaded mesh vertex/index buffers
└── Resources/               - Vulkan resource wrappers
    ├── Buffer.h/cpp         - VMA-backed buffer wrapper
    ├── DescriptorSet*.h/cpp - Descriptor set management
    ├── PipelineLayout.h/cpp - Pipeline layout abstraction
    └── ShaderModule.h/cpp   - Compiled SPIR-V module wrapper
```

### Key Dependencies

- **Vulkan SDK** (with SPIRV-Cross, shaderc)
- **volk** - Vulkan meta-loader
- **vk-bootstrap** - Vulkan initialization helper
- **VMA** (Vulkan Memory Allocator) - Memory management
- **GLFW** - Windowing
- **GLM** - Math library (with Vulkan conventions: depth [0,1], RDF clip-space)
- **Assimp** - Model loading (supports glTF)
- **glaze** - JSON/binary serialization
- **modern-uuid** - UUID generation for assets
- **spdlog** - Logging

## Architecture Patterns

### Asset Pipeline (Two-Phase)

1. **Offline Processing** (`AssetProcessor`):
    - Compiles GLSL → SPIR-V (debug/release variants)
    - Reflects shader schemas (uniforms, inputs, outputs)
    - Processes glTF meshes → binary format
    - Generates asset definitions stored in `processedAssets/` as `.beve` files
    - Maintains registry in `processedAssets/registry.json`

2. **Runtime Loading** (`AssetManager`):
    - Loads processed asset definitions from disk
    - Creates `asset::` objects (lightweight, CPU-side)
    - Caches loaded assets by UUID to avoid duplication

### Render Object Uploading

- `Renderer` has `uploadOrGet()` methods that:
    - Take `Handle<asset::T>` → return `Handle<render::T>`
    - Create GPU resources (buffers, descriptor sets, pipelines)
    - Cache mappings in `std::unordered_map` to avoid re-uploads
    - Example: `asset::Material` → `render::Material` creates UBO + descriptor set

### Handle System

- Type-safe handles via `Handle<T>` (just a wrapper around `u32 id`)
- Handles are stable IDs into `std::vector<T>` storage
- Used for both asset and render objects

### Material System

- **`asset::Material`**:
    - References a `GraphicsProgram` (shader pair)
    - Contains pipeline state (depth test, blending, culling)
    - Stores uniform values as recursive `MaterialUniformValue` (JSON-serialized)
    - Built from `.mat` files (e.g., `debug.mat`)

- **`render::Material`**:
    - Uploads uniform values to UBO
    - Creates descriptor set for material params (set=1, binding=0)
    - Owns a `Pipeline` with full state
    - Provides `setParam()` for runtime uniform updates

### Shader System

- **Vertex inputs**: Up to 10 attributes (position, normal, tangent, color, texCoords, custom)
- **Descriptor sets**:
    - Set 0: Per-frame data (view/projection matrices, frame counter)
    - Set 1: Material parameters (UBO with material-specific uniforms)
- **Push constants**: Transform matrices, mesh/span IDs
- **Reflection**: Uses SPIRV-Cross to extract schemas from compiled shaders

### Pipeline Management

- `PipelineCreateInfo` is hash-able and comparable
- Pipelines cached in `Renderer::pipelineCacheMap_` to avoid duplicates
- Uses `VkPipelineCache` for driver-level caching
- Dynamic state: viewport, scissor (rest is baked)

## Key Code Locations

### Rendering Loop

- `Application::run()` - Main loop at Application.cpp:75
- `Renderer::beginFrame()` - Acquire image, wait for fences
- `Renderer::endFrame()` - Submit commands, present
- Per-frame data setup at Application.cpp:124-139

### Asset Processing

- `AssetProcessor::processAllAssets()` - Full rebuild
- `AssetProcessor::processShaderStage()` - GLSL → SPIR-V compilation
- `AssetProcessor::processGraphicsProgram()` - Shader pair + reflection
- `AssetProcessor::processMaterial()` - Material definition loading
- `AssetProcessor::processMeshes()` - Assimp mesh import

### Shader Reflection

- `reflectShaderStageSchema()` at ShaderReflection.cpp
- Extracts inputs, outputs, uniform buffers from SPIR-V
- `ShaderStageSchema` contains all resource bindings

### Material Creation

- `Material::create()` at AssetMaterial.cpp
- `Material::buildDefaultValues()` - Generates default uniform values from schema
- `render::Material` constructor at RenderMaterial.cpp - GPU upload

## Current State & TODOs (from main.cpp)

### Implemented

✅ GLSL shader compilation with reflection
✅ Graphics program (vert+frag) abstraction
✅ Material system with pipeline state
✅ Mesh loading via Assimp (glTF support)
✅ Asset registry with UUID-based references
✅ Render object caching (shaders, materials, meshes)
✅ Descriptor set management (per-frame, material params)
✅ VMA-backed buffer management
✅ Debug visualization shaders (normals, UVs, vertex colors)

### In Progress / Planned

- [ ] Texture support (asset, render, descriptor updates)
- [ ] More material types (Phong, PBR)
- [ ] Scene abstraction (entity transforms)
- [ ] Material editor UI
- [ ] Normal maps
- [ ] Image-based testing (headless rendering)
- [ ] Tracy profiler integration
- [ ] RenderDoc integration
- [ ] Bindless rendering
- [ ] Advanced techniques: gaussian splats, OIT, differential rendering

## Build Configuration

### CMake Presets

- **Debug**: Full debug info, debug SPIR-V
- **DebugAsan**: Address Sanitizer enabled
- **Release**: Optimized, release SPIR-V
- **RelWithDebInfo**: Optimized with debug symbols

### Important Macros

- `VK_NO_PROTOTYPES` - Using volk for function loading
- `GLM_FORCE_DEPTH_ZERO_TO_ONE` - Vulkan depth range
- `ASSETS_FOLDER` - Points to `src/assets/`

### Compiler Settings

- MSVC `/W4` - High warning level
- `/MP` - Parallel builds
- `/Zc:enumTypes` - Strict enum type checking
- Precompiled headers for STL + key dependencies

## Conventions

### Coordinate Systems

- **Object space**: Defined by artists
- **World space**: Y-up (per GLM)
- **View space**: RUB (Right-Up-Back, GLM default)
- **Clip space**: RDF (Right-Down-Forward, Vulkan convention)
- **Transformation**: Projection matrix [1][1] negated for Y-flip (Application.cpp:139)

### Naming

- `asset::` namespace for CPU-side assets
- `render::` namespace for GPU-side render objects
- `aur::` root namespace
- `Handle<T>` for type-safe IDs
- `StableId<T>` for file-path-based asset identifiers

### File Extensions

- `.vert`, `.frag` - GLSL shader sources
- `.shader` - Graphics program definition (JSON, pairs vert+frag)
- `.mat` - Material definition (JSON, references .shader + pipeline state)
- `.beve` - Binary serialized asset definitions (glaze binary format)

## Current Sample Assets

- **Models**: Box, BoxVertexColors, Duck, DamagedHelmet, etc. (from glTF-Sample-Assets)
- **Shaders**: `debug` (visualization modes), `triangle`, `cube`, `ubo_study`
- **Materials**: `debug.mat` (alpha-blended debug visualization)

---

*This context document helps quickly understand the codebase structure, architecture decisions, and current development
focus for future development sessions.*
