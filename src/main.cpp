/*
TODO(vug): When a DescriptorSet is bound check whether it's compatible with the pipeline's layout.
           (https://gemini.google.com/app/b54463a6f7ddbb82)
TODO(vug): Give vulkan objects debug names. Label CommandBuffer regions. (See chat)
TODO(vug): upload geometry data via Vertex/Index Buffers
TODO(vug): Add Renderer Vertex, Index, Uniform, Storage, Staging buffer creation via VMA methods
TODO(vug): Introduce more resource abstractions (Image/Texture, Sampler... Concrete Buffer types?) then
           introduce CRTP base class generalization. (See chat)
TODO(vug): add Renderer Texture, Depth/Stencil Image, Offscreen Render Target creation via VMA
           methods
TODO(vug): Can I abstract away currentInFlightImageIx and sync objects?
TODO(vug): use slang as the shader language, and import global and per-frame uniforms and default vertex
           attributes etc. from there
TODO(vug): introduce Tracy for frame profiling. Get CPU and GPU work separately.
TODO(vug): generate layouts from shader reflection
TODO(vug): Resource abstractions for Semaphore, Fence, CommandPool, Image(?)
.
TODO(vug): macros: MOVE_ONLY, COPY_ONLY, NOT_MOVABLE_NOT_COPIABLE
TODO(vug): try out C++ modules, but don't obsess if it does not work well
           https://gemini.google.com/app/31dc373a7f5b3005
TODO(vug): consider making all members of dependency library types pointers, so that I can
           forward declare them and won't leak their headers
TODO(vug): add STL includes to a precompiled header pch_stl.h
           Also add headers such as utils and logger that are included in
           every file into pch_aur.h. and maybe a pch_dependencies
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
