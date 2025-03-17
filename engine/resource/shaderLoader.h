#pragma once

#include "api/vulkan/device.h"
#include "common/hash.h"

namespace aph
{
enum class ShaderContainerType
{
    Default,
    Spirv,
    Slang,
};

struct ShaderStageLoadInfo
{
    std::variant<std::string, std::vector<uint32_t>> data;
    std::vector<ShaderMacro> macros;
    std::string entryPoint = "main";
};

struct ShaderLoadInfo
{
    std::string debugName = {};
    HashMap<ShaderStage, ShaderStageLoadInfo> stageInfo;
    std::vector<ShaderConstant> constants;
    vk::BindlessResource* pBindlessResource = {};
};

class ShaderLoader
{
};
} // namespace aph

namespace aph::loader::shader
{
std::vector<uint32_t> loadSpvFromFile(std::string_view filename);
aph::HashMap<aph::ShaderStage, std::pair<std::string, std::vector<uint32_t>>> loadSlangFromFile(
    std::string_view filename);
} // namespace aph::loader::shader
