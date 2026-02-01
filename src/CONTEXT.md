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
├── Handle.h/cpp             - Type-safe handle system for assets/render objects
├── Logger.h/cpp             - Logging utility (spdlog wrapper)
├── FileIO.h/cpp             - File system utilities
├── Utils.h/cpp              - General utilities and helpers
├── Vertex.h/cpp             - Vertex format definitions
├── VulkanWrappers.h/cpp     - Vulkan type forwarding and utilities
├── GlfwUtils.h/cpp          - GLFW utility functions
├── asset/                   - Asset processing and management
│   ├── AssetManager.h/cpp        - Runtime asset loading and caching
│   ├── AssetProcessor.h/cpp      - Offline asset compilation (shaders, meshes)
│   ├── AssetRegistry.h/cpp       - Asset database (processedAssets/registry.json)
│   ├── AssetIds.h/cpp            - Asset ID types (StableId, AssetRef, AssetUuid)
│   ├── AssetConcepts.h           - Asset type concepts and forward declarations
│   ├── AssetTraits.h             - CRTP mixin for asset types (AssetTypeMixin)
│   ├── ShaderReflection.h/cpp    - SPIRV-Cross shader introspection
│   ├── ShaderStage.h/cpp         - Individual shader stage assets
│   ├── GraphicsProgram.h/cpp     - Shader program (vert+frag) assets
│   ├── Material.h/cpp            - Material asset definitions
│   ├── Mesh.h/cpp                - Mesh asset definitions
│   ├── AssimpUtils.h/cpp         - Assimp integration utilities
│   └── AssetTraits.h             - CRTP traits for all asset types
├── render/                   - GPU-side render objects
│   ├── GraphicsProgram.h/cpp     - Uploaded shader programs
│   ├── Material.h/cpp            - Uploaded materials with UBOs
│   ├── Mesh.h/cpp                - Uploaded mesh vertex/index buffers
│   ├── RenderConcepts.h          - Render object type concepts
│   └── RenderGraphicsProgram.cpp - Implementation
├── Resources/                - Vulkan resource wrappers
│   ├── Buffer.h/cpp              - VMA-backed buffer wrapper
│   ├── DescriptorPool.h/cpp      - Descriptor pool management
│   ├── DescriptorSet.h/cpp       - Descriptor set wrapper
│   ├── DescriptorSetLayout.h/cpp - Descriptor set layout creation
│   ├── PipelineLayout.h/cpp      - Pipeline layout abstraction
│   ├── PipelineCache.h/cpp       - Vulkan pipeline cache wrapper
│   ├── ShaderModule.h/cpp        - Compiled SPIR-V module wrapper
│   ├── Allocator.h/cpp           - VMA allocator wrapper
│   └── VulkanResource.h          - CRTP base class for Vulkan resources
└── processedAssets/          - Compiled asset definitions directory
    └── registry.json         - Asset metadata and file listings
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

### Asset Type System (CRTP + Concepts)

The asset system uses **CRTP (Curiously Recurring Template Pattern)** with C++20 **Concepts** to provide:

- **`AssetConcept`**: Constraints for valid asset types (ShaderStage, GraphicsProgram, Material, Mesh)
- **`AssetDefinitionConcept`**: Constraints for asset definition types (structs holding asset data)
- **`AssetTypeMixin<TAsset, TDefinition, ...>`**: CRTP base providing:
    - Static metadata: `label`, `uuidNamespace`, `typeEnum`
    - Cache and storage types
    - Compile-time asset trait introspection
- **`AssetTypeFor<TDefinition>`**: Template specializations mapping definitions to asset types
- **`AssetTraits.h`**: Central registry for all asset CRTP traits

**Benefits**:
- Reduces boilerplate in asset classes
- Type-safe asset handling
- Compile-time verification of asset invariants

### Vulkan Resources (CRTP)

`VulkanResource<Derived, HandleType, CreateInfoType, Contexts...>` provides:
- Move-only semantics for GPU resources
- Context-aware creation/destruction (variadic context types)
- Safe RAII pattern for Vulkan handles

### Asset Pipeline (Two-Phase)

1. **Offline Processing** (`AssetProcessor`):
    - Compiles GLSL → SPIR-V (debug/release variants via `ShaderBuildMode`)
    - Reflects shader schemas (uniforms, inputs, outputs)
    - Processes glTF meshes → binary format (Assimp integration)
    - Generates asset definitions stored in `processedAssets/` as `.beve` files (glaze binary)
    - Maintains registry in `processedAssets/registry.json` with `AssetEntry` metadata

2. **Runtime Loading** (`AssetManager`):
    - Loads processed asset definitions from disk
    - Creates `asset::` objects (lightweight, CPU-side)
    - Caches loaded assets by `AssetUuid` in per-type caches to avoid duplication

### Asset ID System

- **`StableId<T>`**: File-path-based stable IDs (derived from `std::string`)
- **`AssetUuid`**: UUID-based unique identifiers (generated from namespace UUIDs + file paths)
- **`AssetRef`**: References to other assets (with Mode: `ByUuid`, `ByPath`)
- **`AssetType` enum**: Identifies asset class (ShaderStage=0, GraphicsProgram=1, Material=2, Mesh=3)

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

- **`asset::Material`** (CRTP via `AssetTypeMixin`):
    - References a `GraphicsProgram` (shader pair) via `AssetRef`
    - Contains pipeline state (depth test, blending, culling)
    - Stores uniform values as recursive `MaterialUniformValue` (JSON-serialized)
    - Built from `.mat` files (e.g., `debug.mat`)
    - Inherits from `AssetTypeMixin` for type metadata and caching

- **`render::Material`** (Uploaded GPU resource):
    - Uploads uniform values to UBO
    - Creates descriptor set for material params (set=1, binding=0)
    - Owns a `Pipeline` with full state
    - Provides `setParam()` for runtime uniform updates
    - Move-only via Vulkan resource management

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

- `Application::run()` - Main loop at [Application.cpp](Application.cpp#L75)
- `Renderer::beginFrame()` - Acquire image, wait for fences
- `Renderer::endFrame()` - Submit commands, present
- Per-frame data setup at [Application.cpp](Application.cpp#L124-L139)

### Asset Type System

- **CRTP Base**: [AssetTraits.h](asset/AssetTraits.h) - `AssetTypeMixin` template definition
- **Type Concepts**: [AssetConcepts.h](asset/AssetConcepts.h) - `AssetConcept`, `AssetDefinitionConcept` definitions
- **Asset IDs**: [AssetIds.h](asset/AssetIds.h) - `StableId`, `AssetRef`, `AssetUuid` types
- **Render Concepts**: [RenderConcepts.h](render/RenderConcepts.h) - `RenderObjectConcept`
- **Vulkan Resource Base**: [VulkanResource.h](Resources/VulkanResource.h) - CRTP base for GPU resources

### Asset Processing

- `AssetProcessor::processAllAssets()` - Full rebuild at [AssetProcessor.cpp](asset/AssetProcessor.cpp)
- `AssetProcessor::processShaderStage()` - GLSL → SPIR-V compilation
- `AssetProcessor::processGraphicsProgram()` - Shader pair + reflection ([AssetGraphicsProgram.cpp](asset/AssetGraphicsProgram.cpp))
- `AssetProcessor::processMaterial()` - Material definition loading ([AssetMaterial.cpp](asset/AssetMaterial.cpp))
- `AssetProcessor::processMeshes()` - Assimp mesh import via [AssimpUtils.h](asset/AssimpUtils.h)
- `AssetRegistry::loadRegistry()` - Registry JSON loading

### Shader Reflection

- `reflectShaderStageSchema()` at [ShaderReflection.cpp](asset/ShaderReflection.cpp)
- Extracts inputs, outputs, uniform buffers from SPIR-V
- `ShaderStageSchema` contains all resource bindings and vertex attributes

### Material Creation

- `Material::create()` at [AssetMaterial.cpp](asset/AssetMaterial.cpp)
- `Material::buildDefaultValues()` - Generates default uniform values from schema
- `render::Material` constructor at [RenderMaterial.cpp](render/RenderMaterial.cpp) - GPU upload

### Vulkan Resource Management

- **Buffer creation**: [Buffer.h/cpp](Resources/Buffer.h)
- **Descriptor sets**: [DescriptorSet.h](Resources/DescriptorSet.h), [DescriptorSetLayout.h](Resources/DescriptorSetLayout.h)
- **Pipeline cache**: [PipelineCache.h](Resources/PipelineCache.h)
- **Allocator wrapper**: [Allocator.h](Resources/Allocator.h) - VMA integration

## Current State & TODOs

### Implemented

✅ CRTP asset type system with compile-time trait metadata  
✅ C++20 Concepts for type constraints (AssetConcept, RenderObjectConcept, etc.)  
✅ GLSL shader compilation with reflection (debug/release variants)  
✅ Graphics program (vert+frag) abstraction with schema combining  
✅ Material system with pipeline state and uniform parameters  
✅ Mesh loading via Assimp (glTF support) with binary serialization  
✅ Asset registry with UUID-based references and file path mappings  
✅ Render object caching (shaders, materials, meshes)  
✅ Descriptor set management (per-frame, material params, dynamic updates)  
✅ VMA-backed buffer management via `VulkanResource` CRTP  
✅ Debug visualization shaders (normals, UVs, vertex colors)  
✅ File I/O utilities ([FileIO.h](FileIO.h))  
✅ GLFW window and input utilities ([GlfwUtils.h](GlfwUtils.h))  
✅ Logging system ([Logger.h](Logger.h) with spdlog backend)  

### Recent Additions

- **CRTP Asset Type System**: `AssetTypeMixin` reduces boilerplate for all asset classes
- **Concepts**: `AssetConcept`, `RenderObjectConcept`, `AssetDefinitionConcept` for type safety
- **Vulkan Resource CRTP**: `VulkanResource<Derived, HandleType, ...>` for GPU memory management
- **Asset ID Refinement**: Introduced `AssetUuid`, `StableId<T>`, `AssetRef` for flexible asset references
- **Descriptor Pool/Set Abstractions**: Moved from monolithic to dedicated [DescriptorPool.h](Resources/DescriptorPool.h), [DescriptorSet.h](Resources/DescriptorSet.h)
- **Pipeline Cache**: Dedicated [PipelineCache.h](Resources/PipelineCache.h) for `VkPipelineCache` management
- **Allocator Abstraction**: [Allocator.h](Resources/Allocator.h) wraps VMA for cleaner resource creation

### In Progress / Planned

- [ ] Texture support (asset, render, descriptor updates)
- [ ] More material types (Phong, PBR, standard materials)
- [ ] Scene abstraction (entity transforms, entity component system)
- [ ] Material editor UI
- [ ] Normal maps and advanced material texturing
- [ ] Image-based testing (headless rendering)
- [ ] Tracy profiler integration
- [ ] RenderDoc integration
- [ ] Bindless rendering
- [ ] Advanced techniques: gaussian splats, OIT, differential rendering
- [ ] C++ Modules support for dependency management

## Design Notes

### Why CRTP for Assets?

Using CRTP (`AssetTypeMixin`) provides:
- **Zero runtime overhead**: All type metadata is compile-time static
- **Reduced boilerplate**: Common cache/storage patterns in one template
- **Type safety**: `Handle<T>` maps to `T::StorageType` and `T::CacheType` automatically
- **Extensibility**: Adding new asset types requires minimal code (inherit from mixin, specialize `AssetTypeFor`)

### Why Concepts?

C++20 Concepts provide:
- **Compile-time verification**: Only valid asset/render types pass
- **Better error messages**: "doesn't satisfy AssetConcept" is clearer than template errors
- **Dispatch clarity**: Template specializations explicitly list constraints
- **Type hierarchy documentation**: Readers see what types are valid at a glance

### Separation of Concerns

- **`asset::*` (CPU)**:
    - Definition structs: Serializable, hold data
    - Classes: CRTP-based, manage CPU-side state, can be cached
    - Focus: Organization, caching, fast lookups
    
- **`render::*` (GPU)**:
    - Move-only objects
    - Derived from `VulkanResource` for lifetime management
    - Focus: Vulkan state management, GPU synchronization
    
- **`Resources::` (Vulkan primitives)**:
    - Low-level Vulkan resource wrappers
    - CRTP base for common patterns
    - Focus: RAII, context-aware creation/destruction

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

### Asset and Resource Design

- **Asset classes**: Must publicly inherit from `AssetTypeMixin` (CRTP pattern)
- **Asset definitions**: Pure structs (no inheritance) with required `schemaVersion` static member
- **Render objects**: Use `RenderObjectConcept` for compile-time verification
- **Vulkan resources**: Inherit from `VulkanResource<Derived, ...>` (CRTP) for RAII management
- **Concepts**: Used throughout for:
    - `AssetConcept`: Valid asset types
    - `AssetDefinitionConcept`: Valid asset definition types
    - `RenderObjectConcept`: Valid render object types
    - `VulkanHandleConcept`: Valid Vulkan handle types

### Coordinate Systems

- **Object space**: Defined by artists
- **World space**: Y-up (per GLM)
- **View space**: RUB (Right-Up-Back, GLM default)
- **Clip space**: RDF (Right-Down-Forward, Vulkan convention)
- **Transformation**: Projection matrix [1][1] negated for Y-flip (Application.cpp:139)

### Naming

- `asset::` namespace for CPU-side assets (with CRTP via `AssetTypeMixin`)
- `render::` namespace for GPU-side render objects
- `aur::` root namespace
- `Handle<T>` for type-safe IDs (simple wrapper around `u32`)
- `StableId<T>` for file-path-based asset identifiers (derived from `std::string`)
- `AssetUuid` for stable UUID-based identification
- `AssetRef` for cross-asset references
- File naming: snake_case for files, PascalCase for classes
- Files prefixed with domain: `Asset*` (asset system), `Render*` (render objects), `Descriptor*` (Vulkan resources)

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
