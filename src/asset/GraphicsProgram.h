#pragma once

#include <functional>

#include "../Handle.h"
#include "AssetIds.h"
#include "ShaderStage.h"

namespace aur::asset {

struct GraphicsProgramDefinition {
  AssetRef vert;
  AssetRef frag;
};

class GraphicsProgram : public AssetTypeMixin<GraphicsProgram, GraphicsProgramDefinition, "GraphicsProgram"> {
public:
  static GraphicsProgram create(const GraphicsProgramDefinition& programDef, Handle<ShaderStage> vertStage,
                                Handle<ShaderStage> fragStage);

  ~GraphicsProgram() = default;

  GraphicsProgram(const GraphicsProgram& other) = delete;
  GraphicsProgram& operator=(const GraphicsProgram& other) = delete;
  GraphicsProgram(GraphicsProgram&& other) noexcept = default;
  GraphicsProgram& operator=(GraphicsProgram&& other) noexcept = default;

  [[nodiscard]] const GraphicsProgramDefinition& getDefinition() const { return def_; }
  [[nodiscard]] const SpirV& getVertexBlob() const { return vert_.get().getSpirVBlob(); }
  [[nodiscard]] const SpirV& getFragmentBlob() const { return frag_.get().getSpirVBlob(); }
  [[nodiscard]] const std::string& getDebugName() const { return debugName_; }

private:
  GraphicsProgram() = default;

  GraphicsProgramDefinition def_;
  Handle<ShaderStage> vert_;
  Handle<ShaderStage> frag_;
  std::string debugName_;
};

using GraphicsProgramUpdateCallback = std::function<void(Handle<asset::GraphicsProgram>)>;

} // namespace aur::asset
