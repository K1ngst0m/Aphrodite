#include "shaderCache.h"
#include "common/profiler.h"
#include "filesystem/filesystem.h"
#include "global/globalManager.h"
#include "slangLoader.h"

namespace aph
{
ShaderCache::ShaderCache()
{
    APH_PROFILER_SCOPE();

    // Set default cache directory
    auto& fs                 = APH_DEFAULT_FILESYSTEM;
    std::string cacheDirPath = fs.resolvePath("shader_cache://").valueOr("cache/shaders");

    if (!fs.exist(cacheDirPath))
    {
        if (fs.createDirectories(cacheDirPath))
        {
            CM_LOG_INFO("Created shader cache directory: %s", cacheDirPath.c_str());
        }
        else
        {
            CM_LOG_WARN("Failed to create shader cache directory: %s", cacheDirPath.c_str());
        }
    }

    m_cacheDirectory = cacheDirPath;
}

std::shared_future<ShaderCache::ShaderCacheData> ShaderCache::findShader(const std::string& cacheKey)
{
    APH_PROFILER_SCOPE();

    std::lock_guard<std::mutex> lock(m_cacheMutex);

    auto it = m_memoryCache.find(cacheKey);
    if (it != m_memoryCache.end())
    {
        return it->second;
    }

    return {}; // Return empty future if not found
}

bool ShaderCache::checkShaderCache(const CompileRequest& request, std::string& outCachePath) const
{
    APH_PROFILER_SCOPE();

    auto& fs = APH_DEFAULT_FILESYSTEM;

    // First check if the shader_cache protocol is registered
    if (!fs.exist(m_cacheDirectory))
    {
        return false;
    }

    // Generate cache key and check if cache file exists
    std::string cacheKey = generateCacheKey(request);
    outCachePath         = getCacheFilePath(cacheKey);

    return fs.exist(outCachePath);
}

bool ShaderCache::readShaderCache(const std::string& cacheFilePath,
                                  HashMap<aph::ShaderStage, SlangProgram>& spvCodeMap) const
{
    APH_PROFILER_SCOPE();

    auto& fs = APH_DEFAULT_FILESYSTEM;

    auto cacheBytes = fs.readFileToBytes(cacheFilePath);
    if (!cacheBytes.success())
    {
        CM_LOG_WARN("Failed to read cache file: %s - %s", cacheFilePath.c_str(), cacheBytes.error().toString().data());
        return false;
    }

    if (cacheBytes.value().empty())
    {
        CM_LOG_WARN("Empty cache file: %s", cacheFilePath.c_str());
        return false;
    }

    // Parse cache data
    size_t offset = 0;

    // Read number of shader stages
    if (offset + sizeof(uint32_t) > cacheBytes.value().size())
    {
        CM_LOG_WARN("Cache file too small for header: %s", cacheFilePath.c_str());
        return false;
    }

    uint32_t numStages;
    std::memcpy(&numStages, cacheBytes.value().data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    bool cacheValid = true;
    // Read each shader stage data
    for (uint32_t i = 0; i < numStages && cacheValid; ++i)
    {
        // Read stage value and entry point length
        if (offset + sizeof(uint32_t) * 2 > cacheBytes.value().size())
        {
            CM_LOG_WARN("Cache file corrupted: too small for stage header, file: %s", cacheFilePath.c_str());
            cacheValid = false;
            break;
        }

        uint32_t stageVal;
        std::memcpy(&stageVal, cacheBytes.value().data() + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        uint32_t entryPointLength;
        std::memcpy(&entryPointLength, cacheBytes.value().data() + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        aph::ShaderStage stage = static_cast<aph::ShaderStage>(stageVal);

        // Read entry point
        if (offset + entryPointLength > cacheBytes.value().size())
        {
            CM_LOG_WARN("Cache file corrupted: too small for entry point, file: %s", cacheFilePath.c_str());
            cacheValid = false;
            break;
        }
        std::string entryPoint(entryPointLength, '\0');
        std::memcpy(entryPoint.data(), cacheBytes.value().data() + offset, entryPointLength);
        offset += entryPointLength;

        // Read spv code length
        if (offset + sizeof(uint32_t) > cacheBytes.value().size())
        {
            CM_LOG_WARN("Cache file corrupted: too small for code size, file: %s", cacheFilePath.c_str());
            cacheValid = false;
            break;
        }
        uint32_t codeSize;
        std::memcpy(&codeSize, cacheBytes.value().data() + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        // Read spv code
        if (offset + codeSize > cacheBytes.value().size())
        {
            CM_LOG_WARN("Cache file corrupted: too small for SPIR-V code, file: %s", cacheFilePath.c_str());
            cacheValid = false;
            break;
        }
        std::vector<uint32_t> spvCode(codeSize / sizeof(uint32_t));
        std::memcpy(spvCode.data(), cacheBytes.value().data() + offset, codeSize);
        offset += codeSize;

        // Store in spvCodeMap
        spvCodeMap[stage] = {entryPoint, std::move(spvCode)};
    }

    if (!cacheValid)
    {
        // Clear any partial data
        spvCodeMap.clear();
        return false;
    }

    return true;
}

std::string ShaderCache::getCacheFilePath(const std::string& cacheKey) const
{
    return m_cacheDirectory + "/" + cacheKey + ".cache";
}

void ShaderCache::addShader(const std::string& cacheKey, const std::shared_future<ShaderCacheData>& shaderData)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_memoryCache[cacheKey] = shaderData;
}

void ShaderCache::removeShader(const std::string& cacheKey)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_memoryCache.erase(cacheKey);
}

void ShaderCache::setCacheDirectory(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_cacheDirectory = path;
}

std::string ShaderCache::getCacheDirectory() const
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    return m_cacheDirectory;
}

void ShaderCache::clear()
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_memoryCache.clear();
}

std::string ShaderCache::generateCacheKey(const CompileRequest& request) const
{
    return request.getHash();
}

} // namespace aph
