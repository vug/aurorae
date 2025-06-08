// TODO(vug): Renderer::setClearColor function
// TODO(vug): move readFile out of Utils and put it into FileIO.h
//            Consider it returning a fixed sized, simple buffer class, instead
//            of std::vector Also instead of const string& use string_view? and
//            forward declare it?
// TODO(vug): define my own numeric types (i32 etc), put it in Utils.h
// TODO(vug): add STL includes to a precompiled header pch_stl.h
//            Also add headers such as utils and logger that are included in
//            every file into pch_aur.h
// TODO(vug): consider adding init() and deinit()/shutdown() methods to many
// objects/resources so that we can keep them in stack as members without
// needing to put them into pointers
//            But, is there a way to even remove their headers from owning
//            class' header file, e.g. Application owns Window and Renderer,
//            hence we include their headers in app header. Is there a way to
//            not include the headers or forward declare them?
// TODO(vug): go over each file and see issues with design, inclusions, methods,
// members etc.
// TODO(vug): create depth/stencil image of the swapchain (if needed) via VMA.
// TODO(vug): Add Renderer Vertex, Index, Uniform, Storage, Staging buffer
// creation via VMA methods
// TODO(vug): add Renderer Texture, Depth/Stencil Image, Offscreen Render Target
// creation via VMA methods
// TODO(vug): smoother resize (current vkDeviceWaitIdle in recreate causes
// stutter
// TODO(vug): use slang as the shader language
// TODO(vug): config for stopping the debugger at validation issues (via
// std::abort() or similar)
// TODO(vug): is there a way to move Window::init/shutdownGLFW() into
// Application. Window is the second member and initGLFW has to be called before
// it's constructed
#define CROSS_PLATFORM_SURFACE_CREATION

#include "Application.h"
#include "Logger.h"

int main() {
  // We are initializing spdlog and glfw here to reduce the complexity of
  // classes and to decouple glfw initialization/termination from the Window
  // class.
  aur::logInitialize(aur::LogLevel::Trace);
  aur::Window::initGLFW();

  const uint32_t kWidth = 1024;
  const uint32_t kHeight = 768;
  const char* kAppName = "Aurorae";

  aur::Application app(kWidth, kHeight, kAppName);
  app.run();

  aur::Window::shutdownGLFW();
  return 0;
}
