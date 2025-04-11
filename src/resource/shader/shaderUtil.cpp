#include "shaderUtil.h"
#include "api/vulkan/device.h"
#include "common/profiler.h"
#include "filesystem/filesystem.h"
#include "global/globalManager.h"
#include "slangLoader.h"

namespace aph
{

PipelineType determinePipelineType(const HashMap<ShaderStage, vk::Shader*>& shaders)
{
    APH_PROFILER_SCOPE();

    // Check for common pipeline configurations
    if (shaders.contains(ShaderStage::CS))
    {
        return PipelineType::Compute;
    }
    else if (shaders.contains(ShaderStage::MS) && shaders.contains(ShaderStage::FS))
    {
        return PipelineType::Mesh;
    }
    else if (shaders.contains(ShaderStage::VS) && shaders.contains(ShaderStage::FS))
    {
        return PipelineType::Geometry;
    }

    // Default fallback
    return PipelineType::Undefined;
}

SmallVector<vk::Shader*> orderShadersByPipeline(const HashMap<ShaderStage, vk::Shader*>& shaders,
                                                PipelineType pipelineType)
{
    APH_PROFILER_SCOPE();

    SmallVector<vk::Shader*> orderedShaders{};

    switch (pipelineType)
    {
    case PipelineType::Geometry:
        orderedShaders.push_back(shaders.at(ShaderStage::VS));
        orderedShaders.push_back(shaders.at(ShaderStage::FS));
        break;

    case PipelineType::Mesh:
        if (shaders.contains(ShaderStage::TS))
        {
            orderedShaders.push_back(shaders.at(ShaderStage::TS));
        }
        orderedShaders.push_back(shaders.at(ShaderStage::MS));
        orderedShaders.push_back(shaders.at(ShaderStage::FS));
        break;

    case PipelineType::Compute:
        orderedShaders.push_back(shaders.at(ShaderStage::CS));
        break;

    default:
        break;
    }

    return orderedShaders;
}

std::string generateReflectionCachePath(const SmallVector<vk::Shader*>& shaders)
{
    APH_PROFILER_SCOPE();

    // Create a cache directory if it doesn't exist
    std::filesystem::path cacheDir = "cache/shaders";
    if (!std::filesystem::exists(cacheDir))
    {
        std::filesystem::create_directories(cacheDir);
    }

    // Generate a unique hash for this shader program based on its shaders
    std::string hashInput;

    // Include each shader's code and stage in the hash
    for (const auto& shader : shaders)
    {
        // Add shader stage to hash
        hashInput += aph::vk::utils::toString(shader->getStage());

        // Add shader code to hash
        const auto& code = shader->getCode();
        if (!code.empty())
        {
            // Use the first 100 bytes and last 100 bytes as a simple hash identifier
            size_t bytesToHash = std::min<size_t>(100, code.size());
            hashInput.append(reinterpret_cast<const char*>(code.data()), bytesToHash);

            if (code.size() > 200)
            {
                // Add the last bytes as well for better uniqueness
                hashInput.append(reinterpret_cast<const char*>(code.data() + code.size() - bytesToHash), bytesToHash);
            }
        }

        // Add shader entry point name
        hashInput += shader->getEntryPointName();
    }

    // Generate a hash of the input using std::hash
    size_t hash = std::hash<std::string>{}(hashInput);

    // Format the hash as a hexadecimal string
    std::stringstream ss;
    ss << std::hex << hash;

    // Create the cache file path
    return (cacheDir / (ss.str() + ".toml")).string();
}

std::string generateCacheKey(const std::vector<std::string>& shaderPaths,
                             const HashMap<ShaderStage, std::string>& stageInfo)
{
    APH_PROFILER_SCOPE();

    std::stringstream ss;

    // Include all shader paths
    for (const auto& path : shaderPaths)
    {
        ss << path;
    }

    // Include stage information
    for (const auto& [stage, entryPoint] : stageInfo)
    {
        ss << static_cast<int>(stage) << entryPoint;
    }

    // Generate a hash of the combined string
    size_t hash = std::hash<std::string>{}(ss.str());

    // Format the hash as a hexadecimal string
    std::stringstream hashStr;
    hashStr << std::hex << std::setw(16) << std::setfill('0') << hash;

    return hashStr.str();
}

vk::Shader* createShaderFromSPIRV(ThreadSafeObjectPool<vk::Shader>& shaderPool, const std::vector<uint32_t>& spirvCode,
                                  ShaderStage stage, const std::string& entryPoint)
{
    APH_PROFILER_SCOPE();

    if (spirvCode.empty())
    {
        return nullptr;
    }

    vk::ShaderCreateInfo createInfo{
        .code       = spirvCode,
        .entrypoint = entryPoint,
        .stage      = stage,
    };

    return shaderPool.allocate(createInfo);
}

Expected<bool> writeShaderCacheFile(const std::string& cacheFilePath,
                                    const HashMap<ShaderStage, SlangProgram>& spvCodeMap)
{
    APH_PROFILER_SCOPE();

    auto& fs = APH_DEFAULT_FILESYSTEM;

    // Prepare the cache data
    std::vector<uint8_t> cacheData;

    // Reserve some initial space
    cacheData.reserve(1024 * 1024); // 1MB initial reservation

    // Write header (number of shader stages)
    uint32_t numStages = static_cast<uint32_t>(spvCodeMap.size());
    size_t headerSize  = sizeof(uint32_t);
    cacheData.resize(headerSize);
    std::memcpy(cacheData.data(), &numStages, headerSize);

    // Write each shader stage data
    for (const auto& [stage, slangProgram] : spvCodeMap)
    {
        // Write stage header (stage value and entry point length)
        uint32_t stageVal         = static_cast<uint32_t>(stage);
        uint32_t entryPointLength = static_cast<uint32_t>(slangProgram.entryPoint.size());

        size_t stageHeaderSize   = sizeof(uint32_t) * 2;
        size_t stageHeaderOffset = cacheData.size();
        cacheData.resize(stageHeaderOffset + stageHeaderSize);
        std::memcpy(cacheData.data() + stageHeaderOffset, &stageVal, sizeof(uint32_t));
        std::memcpy(cacheData.data() + stageHeaderOffset + sizeof(uint32_t), &entryPointLength, sizeof(uint32_t));

        // Write entry point
        size_t entryPointOffset = cacheData.size();
        cacheData.resize(entryPointOffset + entryPointLength);
        if (entryPointLength > 0)
        {
            std::memcpy(cacheData.data() + entryPointOffset, slangProgram.entryPoint.data(), entryPointLength);
        }

        // Write spv code length
        uint32_t codeSize     = static_cast<uint32_t>(slangProgram.spvCodes.size() * sizeof(uint32_t));
        size_t codeSizeOffset = cacheData.size();
        cacheData.resize(codeSizeOffset + sizeof(uint32_t));
        std::memcpy(cacheData.data() + codeSizeOffset, &codeSize, sizeof(uint32_t));

        // Write spv code
        size_t codeOffset = cacheData.size();
        cacheData.resize(codeOffset + codeSize);
        if (codeSize > 0)
        {
            std::memcpy(cacheData.data() + codeOffset, slangProgram.spvCodes.data(), codeSize);
        }
    }

    // Write the cache file
    auto writeResult = fs.writeBinaryData(cacheFilePath, cacheData.data(), cacheData.size());
    if (!writeResult.success())
    {
        return {Result::RuntimeError, std::format("Failed to write shader cache file: {}", writeResult.toString())};
    }

    return true;
}

} // namespace aph
