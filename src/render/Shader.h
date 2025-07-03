#pragma once

#include "../Handle.h"
#include "../Resources/ShaderModule.h"
#include "../asset/Shader.h"

namespace aur::render {
struct Shader {
  ShaderModule vertModule;
  ShaderModule fragModule;

  Handle<asset::Shader> asset;
};
} // namespace aur::render