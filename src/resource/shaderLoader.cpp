#include "shaderLoader.h"
#include "common/common.h"
#include "common/profiler.h"
#include "filesystem/filesystem.h"
#include "global/globalManager.h"
#include "reflection/shaderReflector.h"
#include "shaderAsset.h"
#include "slangLoader.h"

namespace aph
{
Result ShaderLoader::load(const ShaderLoadInfo& info, ShaderAsset** ppShaderAsset)
{
    APH_PROFILER_SCOPE();

    //
    // 1. Setup and initialization
    //
    auto& fs                      = APH_DEFAULT_FILESYSTEM;
    CompileRequest compileRequest = info.compileRequestOverride;
    bool forceUncached            = info.forceUncached || !compileRequest.slangDumpPath.empty();

    if (info.pBindlessResource)
    {
        compileRequest.addModule("bindless", fs.readFileToString("shader_slang://modules/bindless.slang").value());
        compileRequest.addModule("gen_bindless", info.pBindlessResource->generateHandleSource());
    }

    auto loadShader = [this](const std::vector<uint32_t>& spv, const ShaderStage stage,
                             const std::string& entryPoint = "main") -> vk::Shader*
    {
        APH_PROFILER_SCOPE();
        vk::ShaderCreateInfo createInfo{
            .code       = spv,
            .entrypoint = entryPoint,
            .stage      = stage,
        };
        return m_shaderPools.allocate(createInfo);
    };

    HashMap<ShaderStage, vk::Shader*> requiredShaderList;

    //
    // 2. Shader loading from cache or compilation
    //
    for (const auto& shaderPath : info.data)
    {
        std::shared_future<ShaderCacheData> future;

        // 2.1. Check in-memory cache
        {
            std::unique_lock<std::mutex> lock{m_loadMtx};
            auto cacheIter = m_shaderCaches.find(shaderPath);

            if (cacheIter != m_shaderCaches.end())
            {
                future = cacheIter->second;
                lock.unlock();

                CM_LOG_INFO("use cached shader, %s", shaderPath.c_str());
                const auto& cachedStageMap = future.get();

                for (const auto& [stage, entryPoint] : info.stageInfo)
                {
                    APH_ASSERT(cachedStageMap.contains(stage) &&
                               cachedStageMap.at(stage)->getEntryPointName() == entryPoint);
                    requiredShaderList[stage] = cachedStageMap.at(stage);
                }
                continue;
            }

            // 2.2. Check disk cache
            std::string cacheFilePath;
            auto resolvedPath       = fs.resolvePath(shaderPath);
            compileRequest.filename = resolvedPath.c_str();
            bool cacheExists = !forceUncached && m_pSlangLoaderImpl->checkShaderCache(compileRequest, cacheFilePath);

            if (cacheExists)
            {
                HashMap<ShaderStage, SlangProgram> spvCodeMap;
                if (m_pSlangLoaderImpl->readShaderCache(cacheFilePath, spvCodeMap))
                {
                    ShaderCacheData data;
                    bool allStagesFound = true;

                    for (const auto& [reqStage, reqEntryPoint] : info.stageInfo)
                    {
                        bool stageFound = false;

                        for (const auto& [stage, slangProgram] : spvCodeMap)
                        {
                            if (reqStage == stage && reqEntryPoint == slangProgram.entryPoint)
                            {
                                vk::Shader* shader = loadShader(slangProgram.spvCodes, stage, slangProgram.entryPoint);
                                requiredShaderList[stage] = shader;
                                data[stage]               = shader;
                                stageFound                = true;
                                break;
                            }
                        }

                        if (!stageFound)
                        {
                            allStagesFound = false;
                            break;
                        }
                    }

                    if (allStagesFound)
                    {
                        std::promise<ShaderCacheData> promise;
                        promise.set_value(std::move(data));
                        future                     = promise.get_future().share();
                        m_shaderCaches[shaderPath] = future;

                        CM_LOG_INFO("loaded shader from cache without initialization: %s", shaderPath.c_str());
                        lock.unlock();
                        continue;
                    }
                }
            }

            // Cache miss - prepare for compilation
            std::promise<ShaderCacheData> promise;
            future                     = promise.get_future().share();
            m_shaderCaches[shaderPath] = future;
            lock.unlock();
        }

        // 2.3. Compile shader from source
        APH_VERIFY_RESULT(waitForInitialization());

        HashMap<ShaderStage, SlangProgram> spvCodeMap;
        {
            std::string cacheFilePath;
            auto resolvedPath       = fs.resolvePath(shaderPath);
            compileRequest.filename = resolvedPath.c_str();
            APH_VERIFY_RESULT(m_pSlangLoaderImpl->loadProgram(compileRequest, spvCodeMap));
        }
        if (spvCodeMap.empty())
        {
            return {Result::RuntimeError, "Failed to load slang shader from file."};
        }

        ShaderCacheData data;
        for (const auto& [stage, entryPoint] : info.stageInfo)
        {
            APH_ASSERT(spvCodeMap.contains(stage) && spvCodeMap.at(stage).entryPoint == entryPoint);
            const auto& spv           = spvCodeMap.at(stage).spvCodes;
            vk::Shader* shader        = loadShader(spv, stage, entryPoint);
            requiredShaderList[stage] = shader;
            data[stage]               = shader;
        }

        std::promise<ShaderCacheData> promise;
        promise.set_value(std::move(data));
    }

    //
    // 3. Pipeline type determination and shader ordering
    //
    SmallVector<vk::Shader*> orderedShaders{};
    PipelineType pipelineType = vk::utils::determinePipelineType(requiredShaderList);

    switch (pipelineType)
    {
    case PipelineType::Geometry:
        orderedShaders.push_back(requiredShaderList.at(ShaderStage::VS));
        orderedShaders.push_back(requiredShaderList.at(ShaderStage::FS));
        break;

    case PipelineType::Mesh:
        if (requiredShaderList.contains(ShaderStage::TS))
        {
            orderedShaders.push_back(requiredShaderList.at(ShaderStage::TS));
        }
        orderedShaders.push_back(requiredShaderList.at(ShaderStage::MS));
        orderedShaders.push_back(requiredShaderList.at(ShaderStage::FS));
        break;

    case PipelineType::Compute:
        orderedShaders.push_back(requiredShaderList.at(ShaderStage::CS));
        break;

    default:
        APH_ASSERT(false);
        return {Result::RuntimeError, "Unsupported shader stage combinations."};
    }

    //
    // 4. Shader reflection
    //
    ShaderReflector reflector{};
    vk::ShaderProgram* pProgram = nullptr;

    ReflectRequest reflectRequest = {
        .shaders = orderedShaders,
        .options = {.extractInputAttributes  = true,
                    .extractOutputAttributes = true,
                    .extractPushConstants    = true,
                    .extractSpecConstants    = true,
                    .validateBindings        = true,
                    .enableCaching           = true,
                    .cachePath               = generateReflectionCachePath(&pProgram, orderedShaders)}
    };
    ReflectionResult reflectionResult = reflector.reflect(reflectRequest);

    //
    // 5. Descriptor set layout creation
    //
    SmallVector<vk::DescriptorSetLayout*> setLayouts = {};
    auto activeSets                                  = ShaderReflector::getActiveDescriptorSets(reflectionResult);
    uint32_t numSets                                 = activeSets.size();

    if (auto maxBoundDescSets = m_pDevice->getPhysicalDevice()->getProperties().maxBoundDescriptorSets;
        numSets > maxBoundDescSets)
    {
        VK_LOG_ERR("Number of sets %u exceeds device limit of %u.", numSets, maxBoundDescSets);
    }

    for (uint32_t setIndex : activeSets)
    {
        vk::DescriptorSetLayoutCreateInfo setLayoutCreateInfo{
            .bindings  = ShaderReflector::getLayoutBindings(reflectionResult, setIndex),
            .poolSizes = ShaderReflector::getPoolSizes(reflectionResult, setIndex),
        };

        auto result = m_pDevice->create(setLayoutCreateInfo);
        APH_VERIFY_RESULT(result);
        setLayouts.push_back(result.value());
    }

    //
    // 6. Pipeline layout creation
    //
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{
        .vertexInput       = reflectionResult.vertexInput,
        .pushConstantRange = reflectionResult.pushConstantRange,
        .setLayouts        = std::move(setLayouts),
    };

    auto layoutResult = m_pDevice->create(pipelineLayoutCreateInfo);
    APH_VERIFY_RESULT(layoutResult);
    vk::PipelineLayout* pipelineLayout = layoutResult.value();

    //
    // 7. Final shader program creation
    //
    vk::ProgramCreateInfo programCreateInfo{.shaders         = std::move(requiredShaderList),
                                            .pPipelineLayout = pipelineLayout};

    auto programResult = m_pDevice->create(programCreateInfo);
    APH_VERIFY_RESULT(programResult);
    pProgram = programResult.value();

    //
    // 8. Create and setup the shader asset
    //
    *ppShaderAsset = m_shaderAssetPools.allocate();

    // Set the program and load info
    (*ppShaderAsset)->setShaderProgram(pProgram);

    // Create a source description from the shader paths
    std::string sourceDesc;
    for (size_t i = 0; i < info.data.size(); ++i)
    {
        if (i > 0)
            sourceDesc += ", ";
        sourceDesc += info.data[i];
    }

    (*ppShaderAsset)->setLoadInfo(sourceDesc, info.debugName);

    return Result::Success;
}

ShaderLoader::~ShaderLoader()
{
    for (auto [_, shaderStageMaps] : m_shaderCaches)
    {
        for (auto [_, shader] : shaderStageMaps.get())
        {
            m_shaderPools.free(shader);
        }
    }
    m_shaderPools.clear();
    m_shaderAssetPools.clear();
}

ShaderLoader::ShaderLoader(vk::Device* pDevice)
    : m_pDevice(pDevice)
    , m_pSlangLoaderImpl(std::make_unique<SlangLoaderImpl>())
{
    // Start the initialization task in the background
    auto& taskManager = APH_DEFAULT_TASK_MANAGER;
    auto taskGroup    = taskManager.createTaskGroup("SlangInitialization");
    taskGroup->addTask(m_pSlangLoaderImpl->initialize());
    m_initFuture = taskGroup->submitAsync();
}

// Add a wait for initialization method
Result ShaderLoader::waitForInitialization()
{
    if (m_initFuture.valid())
    {
        return m_initFuture.get();
    }
    return Result::Success;
}

std::string ShaderLoader::generateReflectionCachePath(vk::ShaderProgram** ppProgram,
                                                      const SmallVector<vk::Shader*>& shaders)
{
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
} // namespace aph
