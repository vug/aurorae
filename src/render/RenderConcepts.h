#pragma once

#include <type_traits>

namespace aur {
namespace render {
class GraphicsProgram;
class Material;
class Mesh;
} // namespace render

template <typename TRenderObj>
concept RenderObjectConcept =
    std::is_same_v<TRenderObj, render::GraphicsProgram> || std::is_same_v<TRenderObj, render::Material> ||
    std::is_same_v<TRenderObj, render::Mesh>;
}; // namespace aur