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

struct CompileRequest
{
    std::string filename;
    HashMap<std::string, std::string> moduleMap;
    std::string_view spvDumpPath;
    std::string_view slangDumpPath;
    bool forceUncached = false;

    template <typename T, typename U>
    void addModule(T&& name, U&& source);

    auto getHash() const -> std::string;
};

struct ShaderLoadInfo
{
    std::string debugName = {};
    std::vector<std::string> data;
    HashMap<ShaderStage, std::string> stageInfo;
    ShaderContainerType containerType       = ShaderContainerType::Default;
    vk::BindlessResource* pBindlessResource = {};
    CompileRequest compileRequestOverride;
    bool forceUncached = false;
};

class SlangLoaderImpl;
class ShaderAsset;
class ShaderCache;

class ShaderLoader
{
public:
    explicit ShaderLoader(vk::Device* pDevice);

    ~ShaderLoader();

    auto load(const ShaderLoadInfo& loadInfo, ShaderAsset** ppShaderAsset) -> Result;

private:
    auto waitForInitialization() -> Result;

private:
    vk::Device* m_pDevice = {};
    ThreadSafeObjectPool<vk::Shader> m_shaderPools;
    ThreadSafeObjectPool<ShaderAsset> m_shaderAssetPools;
    std::mutex m_loadMtx;
    std::unique_ptr<SlangLoaderImpl> m_pSlangLoaderImpl = {};
    std::unique_ptr<ShaderCache> m_pShaderCache         = {};
    std::future<Result> m_initFuture;
};

} // namespace aph

template <typename T, typename U>
inline void aph::CompileRequest::addModule(T&& name, U&& source)
{
    moduleMap[std::forward<T>(name)] = std::forward<U>(source);
}
