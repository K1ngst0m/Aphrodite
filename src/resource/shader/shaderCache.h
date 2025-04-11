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
    // Constructor
    ShaderCache();

    // Typedef for caching shader modules by stage
    using ShaderCacheData = HashMap<ShaderStage, vk::Shader*>;

    // Find shader in memory cache
    std::shared_future<ShaderCacheData> findShader(const std::string& cacheKey);

    // Check if shader exists in file cache
    bool checkShaderCache(const CompileRequest& request, std::string& outCachePath) const;

    // Read shader cache data from file
    bool readShaderCache(const std::string& cacheFilePath, HashMap<aph::ShaderStage, SlangProgram>& spvCodeMap) const;

    // Get cache file path
    std::string getCacheFilePath(const std::string& cacheKey) const;

    // Add shader to memory cache
    void addShader(const std::string& cacheKey, const std::shared_future<ShaderCacheData>& shaderData);

    // Remove shader from memory cache
    void removeShader(const std::string& cacheKey);

    // Set cache directory
    void setCacheDirectory(const std::string& path);

    // Get cache directory
    std::string getCacheDirectory() const;

    // Clear memory cache (doesn't affect file cache)
    void clear();

    // Generate a cache key based on compile request
    std::string generateCacheKey(const CompileRequest& request) const;

private:
    std::string m_cacheDirectory;
    HashMap<std::string, std::shared_future<ShaderCacheData>> m_memoryCache;
    mutable std::mutex m_cacheMutex;
};
} // namespace aph