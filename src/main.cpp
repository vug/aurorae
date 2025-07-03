/*
TODO(vug): item count or size to Buffer
TODO(vug): make createInfo and handle members non-const
TODO(vug): constructors for render::Shader and render::Mesh instead of Renderer::upload functions
TODO(vug): Bring DrawSpans to render::Mesh.
TODO(vug): continue going over clang-tidy settings from modernize-avoid-bind.
           https://clang.llvm.org/extra/clang-tidy/checks/list.html
TODO(vug): Introduce the PipelineCache.
TODO(vug): store debug names with objects
TODO(vug): Introduce asset::Material, render::Material
TODO(vug): First create (dummy) Materials (with different debug names) -> store them in AssetManager
           then create asset::Mesh and MaterialSpans.
TODO(vug): Introduce the drawMesh, drawSubmesh(MeshHandle, drawSpanIx) etc. functions
TODO(vug): introduce Tracy for frame profiling. Get CPU and GPU work separately.
TODO(vug): add STL includes to a precompiled header pch_stl.h
           Also add headers such as utils and logger that are included in
           every file into pch_aur.h. and maybe a pch_dependencies
TODO(vug): bring a texture (stb or oiio)
TODO(vug): introduce RenderDoc
TODO(vug): Apparently const_cast a const member is really UB and I shouldn't do it. Migrate to const getter
           for handles and create infos.
TODO(vug): Introduce a default material, and a missing material.
TODO(vug): Introduce AssetManager that stores assets in unordered_maps to shared_ptr of asset type. It's
           responsible of loading and unloading of assets and keeping them alive.
TODO(vug): CommandBuffer abstraction: takes "oneShot" bool parameter. Has `begin()`, `end()`, `submit()`
           methods.
TODO(vug): Resource abstractions for Semaphore, Fence, CommandPool, DescriptorPool, Image(?)
TODO(vug): bring Open Image IO (does not have to support all formats/backends. One HDR, one lossless, and one
           compressed is enough)
TODO(vug): Very simple scene abstraction that has a vector of meshes.
TODO(vug): Add Renderer Vertex, Index, Uniform, Storage, Staging buffer creation via VMA methods (?)
TODO(vug): Introduce more resource abstractions (Image/Texture, Sampler... Concrete Buffer types?) then
           introduce CRTP base class generalization. (See chat) I can give VulkanContext as context so that
           everything necessary will be available for creation
TODO(vug): add Renderer Texture, Depth/Stencil Image, Offscreen Render Target creation via VMA
           methods
TODO(vug): Can I abstract away currentInFlightImageIx and sync objects?
TODO(vug): use slang as the shader language, and import global and per-frame uniforms and default vertex
           attributes etc. from there
TODO(vug): generate layouts from shader reflection
TODO(vug): basic geometry assets: quad
TODO(vug): introduce entity, so that two entities can use the same mesh/model but have different transforms
.
TODO(vug): Initially Material can be stored in files. Later, split materials and pipelines. Store pipelines in
           files, and store materials in files, where they refer to a pipeline and has some parameters.
TODO(vug): headless vulkan for image based testing.
TODO(vug): macros: MOVE_ONLY, COPY_ONLY, NOT_MOVABLE_NOT_COPIABLE
TODO(vug): try out C++ modules, but don't obsess if it does not work well
           https://gemini.google.com/app/31dc373a7f5b3005
TODO(vug): consider making all members of dependency library types pointers, so that I can
           forward declare them and won't leak their headers
TODO(vug): looks like at app start, longest duration is spent on graphics pipeline creation.
           Add a timer to measure important perf durations (cold start etc)
           Investigate what can be done to make pipeline creation faster. Can we cache them?
TODO(vug): smoother resize (current vkDeviceWaitIdle in recreate causes stutter)
TODO(vug): normal maps
TODO(vug): Indirect rendering (various brush shapes for painterly render), 2D/3D gaussian splats,
           order independent transparency, differential rendering, bring Mitsuba, PBRT etc
TODO(vug): Whenever updating GPU drivers, check whether `vkCmdPushConstants2` is now available [issue #7]
*/

#include "Application.h"

namespace aur {
void main() {
  Application app(1024, 768, "Aurorae");
  app.run();
}
} // namespace aur

int main() {
  aur::main();
  return 0;
}
