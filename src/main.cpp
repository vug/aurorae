/*
TODO(vug): asset::Material owns a PipelineCreateInfo prepared from MaterialDefinition. Calculates it's cache
TODO(vug): render::Material uses the hash of PipelineCreateInfo to load Pipeline if cached on disk
           otherwise create one and store to disk (do storing operations on a separate thread)
TODO(vug): Introduce drawSubmesh(MeshHandle, drawSpanIx) function. Give pipeline and index range to
           drawIndexed.
TODO(vug): Add attributes to fat vertex. (also add meshId too)
TODO(vug): Introduce a phong material with a distant light embedded in the shader
TODO(vug): introduce Tracy for frame profiling. Get CPU and GPU work separately.
TODO(vug): Once all assets are processed, go over the material assets and create render::Materials.
           After that, store vulkan cache to disk. Next time load it. It'll be used at next VkPipeline
           creation!
TODO(vug): turn on SIMD for glaze https://github.com/stephenberry/glaze?tab=readme-ov-file#simd-cmake-options
TODO(vug): do the dynamic vs. baked state suggestions for the pipeline
TODO(vug): Introduce more resource abstractions (Image, ImageView, Sampler... Concrete Buffer types?)
TODO(vug): asset::Texture, render::Texture
TODO(vug): bring a Khronos sample asset with textures
TODO(vug): bring a texture (stb or oiio)
TODO(vug): processOnlyNeedingAssets
TODO(vug): go over more ideas from more_on_registry_and_build_mode_variants.md
TODO(vug): store debug names with objects
TODO(vug): a std::function based listener system for asset changes
TODO(vug): make registry injection easier (maybe use AppContext?)
TODO(vug): Create some default Materials (with different debug names) -> store them in AssetManager
TODO(vug): continue going over clang-tidy settings from performance-.
           https://clang.llvm.org/extra/clang-tidy/checks/list.html
TODO(vug): Also add frequent aur headers such as utils and logger to PCH.
           (Can I have multiple PCH groups in CMake?)
TODO(vug): introduce RenderDoc
TODO(vug): Material processing: 1) infer: "material[{}]{}", m->mMaterialIndex,
           scene->mMaterials[m->mMaterialIndex]->GetName().C_Str() 2) custom renderer materials:
           `glowing_embers.aurmat` 3) Mapped materials: given name -> aurmat
TODO(vug): Very simple scene abstraction that has a vector entities w/render mesh handles and transforms.
TODO(vug): try out C++ modules, but don't obsess if it does not work well
           https://gemini.google.com/app/31dc373a7f5b3005
TODO(vug): Introduce a missing material.
TODO(vug): Renderer also has a cache for render objects it uploaded from asset handles.
TODO(vug): CommandBuffer abstraction: takes "oneShot" bool parameter. Has `begin()`, `end()`, `submit()`
           methods.
TODO(vug): Resource abstractions for Semaphore, Fence, CommandPool, DescriptorPool, Image(?)
TODO(vug): bring Open Image IO (does not have to support all formats/backends. One HDR, one lossless, and one
           compressed is enough)
TODO(vug): Add Renderer Vertex, Index, Uniform, Storage, Staging buffer creation via VMA methods (?)
TODO(vug)  introduce CRTP Resource base class generalization. (See chat) I can give VulkanContext as context
           so that everything necessary will be available for creation
TODO(vug): add Depth/Stencil Image, Offscreen Render Target creation via VMA
           methods
TODO(vug): Can I abstract away currentInFlightImageIx and sync objects?
TODO(vug): use slang as the shader language, and import global and per-frame uniforms and default vertex
           attributes etc. from there
TODO(vug): generate layouts from shader reflection
TODO(vug): basic geometry assets: quad
TODO(vug): ability to delete asset and render objects. Need an HandleManager that'll mark removed obj indices.
.
TODO(vug): Initially Material can be stored in files. Later, split materials and pipelines. Store pipelines in
           files, and store materials in files, where they refer to a pipeline and has some parameters.
TODO(vug): headless vulkan for image based testing.
TODO(vug): macros: MOVE_ONLY, COPY_ONLY, NOT_MOVABLE_NOT_COPIABLE
TODO(vug): consider making all members of dependency library types pointers, so that I can
           forward declare them and won't leak their headers
TODO(vug): looks like at app start, longest duration is spent on graphics pipeline creation.
           Add a timer to measure important perf durations (cold start etc)
TODO(vug): smoother resize (current vkDeviceWaitIdle in recreate causes stutter)
.
TODO(vug): Material Editor, Asset Viewer
TODO(vug): normal maps
TODO(vug): Indirect rendering (various brush shapes for painterly render), 2D/3D gaussian splats,
           order independent transparency, differential rendering, bring Mitsuba, PBRT etc
TODO(vug): Whenever updating GPU drivers, check whether `vkCmdPushConstants2` is now available [issue #7]
TODO(vug): do screen-space GI Global Illumination for Poor People | TurboGI Devlog - YouTube
           https://www.youtube.com/watch?v=dmdyqzelBIY
TODO(vug): Consider using https://en.cppreference.com/w/cpp/execution.html,
           here is an implementation https://github.com/NVIDIA/stdexec
TODO(vug): bring https://github.com/Neargye/magic_enum and use it for auto enum -> string conversions.
           and https://github.com/getml/reflect-cpp for general reflection <- I can use glaze in the meantime
TODO(vug): Add definitions to JSON schemas https://github.com/stephenberry/glaze/blob/main/docs/json-schema.md
           (and make a CLI tool?)
*/

#include "Application.h"

int main() {
  aur::Application app(1024, 768, "Aurorae");
  app.run();
  return 0;
}
