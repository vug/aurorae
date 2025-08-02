#pragma once

#include <filesystem>
#include <functional>

#include "../Handle.h"
#include "ShaderStage.h"

#include "Common.h"

namespace aur::asset {

struct GraphicsProgramDefinition {
  StableId<ShaderStageDefinition> vert;
  StableId<ShaderStageDefinition> frag;
};

class GraphicsProgram {
public:
  static GraphicsProgram create(const GraphicsProgramDefinition& shaderDef, Handle<ShaderStage> vertStage,
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

using ShaderUpdateCallback = std::function<void(Handle<asset::GraphicsProgram>)>;

} // namespace aur::asset
