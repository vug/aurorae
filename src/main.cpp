// TODO(vug): DescriptorLayoutSet comparison (isEqual and isCompatible)
// https://gemini.google.com/app/b54463a6f7ddbb82
// TODO(vug): DescriptorSet abstraction. Allocated via pool for a layout. check whether it's compatible with
//            the layout.
// TODO(vug): Give vulkan objects debug names. Label CommandBuffer regions. (See chat)
// TODO(vug): Can I abstract away currentInFlightImageIx and sync objects?
// TODO(vug): upload geometry data via Vertex/Index Buffers
// TODO(vug): Add Renderer Vertex, Index, Uniform, Storage, Staging buffer creation via VMA methods
// TODO(vug): consider making all members of dependency library types pointers, so that I can
// forward declare them and won't leak their headers
// TODO(vug): Go over former commits and create issues and milestones for them.
// TODO(vug): try out C++ modules, but don't obsess if it does not work well
//            https://gemini.google.com/app/31dc373a7f5b3005
// TODO(vug): add STL includes to a precompiled header pch_stl.h
//            Also add headers such as utils and logger that are included in
//            every file into pch_aur.h. and maybe a pch_dependencies
// TODO(vug): add Renderer Texture, Depth/Stencil Image, Offscreen Render Target creation via VMA
// methods
// TODO(vug): introduce clang-tidy. Apply modernizers etc. (Maybe don't run it from VSCode but from
//            command-line?) trailing return types etc.
// TODO(vug): RAII wrappers for Buffer that takes VmaAllocator, VkDeviceSize, VkBufferUsageFlags
// (whatever necessary)
//            move-only (no copy). Similar for Image, ImageView, Sampler. Manages single Vulkan
//            handle. can have extra methods, e.g. Buffer can have map/unmap, flush
// TODO(vug): two-phase initialization (construct -> init. deinit -> deconstruct) for graphics
// pipeline, and mesh.
//            They store references to device and VmaAllocator etc. Mesh can have initFromData, and
//            loadFromFile.
// TODO(vug): use slang as the shader language
// TODO(vug): smoother resize (current vkDeviceWaitIdle in recreate causes stutter
// TODO(vug): looks like at app start, longest duration is spent on graphics pipeline creation.
//            Add a timer to measure important perf durations (cold start etc)
//            Investigate what can be done to make pipeline creation faster. Can we cache them?
// TODO(vug): normal maps
// TODO(vug): Indirect rendering (various brush shapes for painterly render), 2D/3D gaussian splats,
//            order independent transparency, differential rendering, bring Mitsuba, PBRT etc
// TODO(vug): Whenever updating GPU drivers, check whether `vkCmdPushConstants2` is now available

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
