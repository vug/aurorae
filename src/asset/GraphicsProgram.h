#pragma once

#include <functional>

#include "../Handle.h"
#include "AssetIds.h"
#include "ShaderStage.h"

namespace aur::asset {

struct GraphicsProgramDefinition {
  // Increment version after each change to the schema or processing logic
  static constexpr u32 schemaVersion{1};
  u32 version{schemaVersion};

  AssetRef vert;
  AssetRef frag;
};

class GraphicsProgram
    : public AssetTypeMixin<GraphicsProgram, GraphicsProgramDefinition, AssetType::GraphicsProgram,
                            "GraphicsProgram", "019870da-2c87-7f9e-aece-9484ce47cac9"> {
public:
  static GraphicsProgram create(Handle<ShaderStage> vertStage, Handle<ShaderStage> fragStage);

  ~GraphicsProgram() = default;
  GraphicsProgram(const GraphicsProgram& other) = delete;
  GraphicsProgram& operator=(const GraphicsProgram& other) = delete;
  GraphicsProgram(GraphicsProgram&& other) noexcept = default;
  GraphicsProgram& operator=(GraphicsProgram&& other) noexcept = default;

  [[nodiscard]] const SpirV& getVertexBlob() const { return vert_.get().getSpirVBlob(); }
  [[nodiscard]] const SpirV& getFragmentBlob() const { return frag_.get().getSpirVBlob(); }
  [[nodiscard]] const std::string& getDebugName() const { return debugName_; }

private:
  GraphicsProgram() = default;
  std::string debugName_;

  Handle<ShaderStage> vert_;
  Handle<ShaderStage> frag_;
};

using GraphicsProgramUpdateCallback = std::function<void(Handle<asset::GraphicsProgram>)>;

} // namespace aur::asset
