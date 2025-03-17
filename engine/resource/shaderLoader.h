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

struct ShaderLoadInfo
{
    std::string debugName = {};
    std::vector<std::string> data;
    HashMap<ShaderStage, std::string> stageInfo;
    ShaderContainerType containerType = ShaderContainerType::Default;
    vk::BindlessResource* pBindlessResource = {};
};

class SlangLoaderImpl;

class ShaderLoader
{
public:
    ShaderLoader(vk::Device* pDevice, ShaderLoadInfo loadInfo);

    ~ShaderLoader();

    Result load(vk::ShaderProgram** ppProgram);

private:
    std::unique_ptr<SlangLoaderImpl> m_pSlangLoaderImpl = {};
    ShaderLoadInfo m_loadInfo;
    vk::Device* m_pDevice = {};
};

} // namespace aph
