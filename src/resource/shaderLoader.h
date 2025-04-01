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
    ShaderLoader(vk::Device* pDevice);

    ~ShaderLoader();

    Result load(const ShaderLoadInfo& loadInfo, vk::ShaderProgram** ppProgram);

private:
    Result waitForInitialization();

private:
    vk::Device* m_pDevice = {};
    ThreadSafeObjectPool<vk::Shader> m_shaderPools;
    using ShaderCacheData = HashMap<ShaderStage, vk::Shader*>;
    HashMap<std::filesystem::path, std::shared_future<ShaderCacheData>> m_shaderCaches;
    std::mutex m_loadMtx;
    std::unique_ptr<SlangLoaderImpl> m_pSlangLoaderImpl = {};
    std::future<Result> m_initFuture;
};

} // namespace aph
