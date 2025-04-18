#pragma once

#include "common/common.h"
#include "common/hash.h"
#include "shaderAsset.h"
#include "shaderLoader.h"

namespace aph
{
// Forward declarations
struct SlangProgram;

namespace vk
{
class Shader;
}

// Shader cache manager
class ShaderCache
{
public:
    ShaderCache();

    // Typedef for caching shader modules by stage
    using ShaderCacheData = HashMap<ShaderStage, vk::Shader*>;

    // Core cache operations
    auto findShader(const std::string& cacheKey) -> std::shared_future<ShaderCacheData>;
    void addShader(const std::string& cacheKey, const std::shared_future<ShaderCacheData>& shaderData);
    void removeShader(const std::string& cacheKey);
    void clear();

    // File cache operations
    auto checkShaderCache(const CompileRequest& request, std::string& outCachePath) const -> bool;
    auto readShaderCache(const std::string& cacheFilePath, HashMap<aph::ShaderStage, SlangProgram>& spvCodeMap) const
        -> bool;
    auto getCacheFilePath(const std::string& cacheKey) const -> std::string;

    // Cache configuration
    void setCacheDirectory(const std::string& path);
    auto getCacheDirectory() const -> std::string;
    auto generateCacheKey(const CompileRequest& request) const -> std::string;

private:
    std::string m_cacheDirectory;
    HashMap<std::string, std::shared_future<ShaderCacheData>> m_memoryCache;
    mutable std::mutex m_cacheMutex;
};
} // namespace aph
