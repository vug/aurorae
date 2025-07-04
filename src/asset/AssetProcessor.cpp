#include "AssetProcessor.h"
#include "../FileIO.h"
#include "../Logger.h"
#include "../Utils.h"

namespace aur {
asset::ShaderDefinition AssetProcessor::processShader(const std::filesystem::path& vertPath,
                                                      const std::filesystem::path& fragPath) {

  if (!std::filesystem::exists(vertPath) || !std::filesystem::exists(fragPath))
    return {};

  const asset::ShaderDefinition def{
      .vertPath = vertPath,
      .fragPath = fragPath,
      .vertBlob = readBinaryFile(vertPath),
      .fragBlob = readBinaryFile(fragPath),
  };

  if (!validateSPIRV(def.vertBlob) || !validateSPIRV(def.fragBlob))
    return {};

  return def;
}

constexpr u32 kSpirVMagic = 0x07230203; // SPIR-V magic number
std::string getSpirvVersionString(u32 version) {
  const u32 major = (version >> 16) & 0xFFFF; // Upper 16 bits
  const u32 minor = (version >> 8) & 0xFF;    // Next 8 bits
  return std::to_string(major) + "." + std::to_string(minor);
}
std::string getSpirvGeneratorString(uint32_t generator) {
  const u16 vendorID = static_cast<u16>(generator >> 16);
  const u16 toolVersion = static_cast<u16>(generator & 0xFFFF);

  // Map Vendor ID to human-readable names from https://registry.khronos.org/SPIR-V/api/spir-v.xml
  std::string vendorName;
  // clang-format off
  switch (vendorID) {
    case 0: vendorName = "Khronos"; break;
    case 1: vendorName = "LunarG"; break;
    case 2: vendorName = "Valve"; break;
    case 3: vendorName = "Codeplay"; break;
    case 4: vendorName = "NVIDIA"; break;
    case 5: vendorName = "ARM"; break;
    case 6: vendorName = "Khronos - LLVM Generator"; break;
    case 7: vendorName = "Khronos - Assembler"; break;
    case 8: vendorName = "Khronos - Glslang"; break;
    case 17: vendorName = "Khronos - Linker"; break;
    default: vendorName = "Unknown Vendor"; break;
  }
  // clang-format on
  return vendorName + " (Version " + std::to_string(toolVersion) + ")";
}

bool AssetProcessor::validateSPIRV(const std::vector<std::byte>& blob) {
  // A SPIR-V should have at least the first 5 words (magic, version, generator, bound, schema)
  if (blob.size() < sizeof(u32) * 5)
    return false;

  // Interpret the blob as u32 words
  const auto words = reinterpret_cast<const u32*>(blob.data());

  if (words[0] != kSpirVMagic)
    return false;

  const u32 version = words[1];
  const u32 generator = words[2];
  const u32 bound = words[3];
  const u32 schema = words[4];
  if (schema != 0) {
    log().warn("SPIR-V schema has to be 0, but is {}.", schema);
    return false;
  }
  log().debug("SPIR-V version: {}, generator: {}, bound: {}", getSpirvVersionString(version),
              getSpirvGeneratorString(generator), bound);

  // Ensure the size of the blob is valid (must contain complete 32-bit words)
  if (blob.size() % sizeof(u32) != 0)
    return false;

  return true;
}

} // namespace aur