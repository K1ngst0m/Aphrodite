#include "shaderLoader.h"
#include "common/profiler.h"
#include "filesystem/filesystem.h"
#include "reflection/shaderReflector.h"
#include "slangLoader.h"

namespace aph
{
Result ShaderLoader::load(const ShaderLoadInfo& info, vk::ShaderProgram** ppProgram)
{
    APH_PROFILER_SCOPE();

    auto& fs = APH_DEFAULT_FILESYSTEM;

    CompileRequest compileRequest = info.compileRequestOverride;

    // Determine if we should force uncached compilation for debugging
    bool forceUncached = info.forceUncached || !compileRequest.slangDumpPath.empty();

    if (info.pBindlessResource)
    {
        // TODO unused since the warning suppress compiler option not working
        compileRequest.addModule("bindless", fs.readFileToString("shader_slang://modules/bindless.slang"));
        compileRequest.addModule("gen_bindless", info.pBindlessResource->generateHandleSource());
    }

    auto loadShader = [this](const std::vector<uint32_t>& spv, const ShaderStage stage,
                             const std::string& entryPoint = "main") -> vk::Shader*
    {
        APH_PROFILER_SCOPE();
        vk::Shader* shader;
        vk::ShaderCreateInfo createInfo{
            .code = spv,
            .entrypoint = entryPoint,
            .stage = stage,
        };
        shader = m_shaderPools.allocate(createInfo);
        return shader;
    };

    HashMap<ShaderStage, vk::Shader*> requiredShaderList;
    for (const auto& d : info.data)
    {
        std::shared_future<ShaderCacheData> future;
        {
            std::unique_lock<std::mutex> lock{m_loadMtx};

            if (auto it = m_shaderCaches.find(d); it == m_shaderCaches.end())
            {
                // Check if we can use the cache directly without initialization
                std::string cacheFilePath;
                auto path = fs.resolvePath(d);
                compileRequest.filename = path.c_str();
                bool cacheExists =
                    !forceUncached && m_pSlangLoaderImpl->checkShaderCache(compileRequest, cacheFilePath);

                if (cacheExists)
                {
                    HashMap<ShaderStage, SlangProgram> spvCodeMap;
                    if (m_pSlangLoaderImpl->readShaderCache(cacheFilePath, spvCodeMap))
                    {
                        ShaderCacheData data;

                        for (const auto& [stage, slangProgram] : spvCodeMap)
                        {
                            for (const auto& [reqStage, reqEntryPoint] : info.stageInfo)
                            {
                                if (reqStage == stage && reqEntryPoint == slangProgram.entryPoint)
                                {
                                    vk::Shader* shader =
                                        loadShader(slangProgram.spvCodes, stage, slangProgram.entryPoint);
                                    requiredShaderList[stage] = shader;
                                    data[stage] = shader;
                                    break;
                                }
                            }
                        }

                        std::promise<ShaderCacheData> promise;
                        promise.set_value(std::move(data));
                        future = promise.get_future().share();
                        m_shaderCaches[d] = future;

                        CM_LOG_INFO("loaded shader from cache without initialization: %s", d.c_str());
                        continue;
                    }
                }

                // Cache doesn't exist or is invalid
                std::promise<ShaderCacheData> promise;
                future = promise.get_future().share();
                m_shaderCaches[d] = future;
                lock.unlock();

                {
                    // Create global session only when cache missed
                    APH_VR(waitForInitialization());

                    HashMap<ShaderStage, SlangProgram> spvCodeMap;
                    APH_VR(m_pSlangLoaderImpl->loadProgram(compileRequest, spvCodeMap));
                    if (spvCodeMap.empty())
                    {
                        return {Result::RuntimeError, "Failed to load slang shader from file."};
                    }

                    ShaderCacheData data;
                    for (const auto& [stage, entryPoint] : info.stageInfo)
                    {
                        APH_ASSERT(spvCodeMap.contains(stage) && spvCodeMap.at(stage).entryPoint == entryPoint);
                        const auto& spv = spvCodeMap.at(stage).spvCodes;
                        vk::Shader* shader = loadShader(spv, stage, entryPoint);
                        requiredShaderList[stage] = shader;
                        data[stage] = shader;
                    }
                    promise.set_value(std::move(data));
                }
            }
            else
            {
                future = it->second;
                CM_LOG_INFO("use cached shader, %s", d);
                for (const auto& [stage, entryPoint] : info.stageInfo)
                {
                    const auto& cachedStageMap = future.get();
                    APH_ASSERT(cachedStageMap.contains(stage) &&
                               cachedStageMap.at(stage)->getEntryPointName() == entryPoint);
                    requiredShaderList[stage] = cachedStageMap.at(stage);
                }
            }
        }
    }

    {
        SmallVector<vk::Shader*> shaders{};

        // vs + fs
        if (requiredShaderList.contains(ShaderStage::VS) && requiredShaderList.contains(ShaderStage::FS))
        {
            shaders.push_back(requiredShaderList.at(ShaderStage::VS));
            shaders.push_back(requiredShaderList.at(ShaderStage::FS));
        }
        else if (requiredShaderList.contains(ShaderStage::MS) && requiredShaderList.contains(ShaderStage::FS))
        {
            if (requiredShaderList.contains(ShaderStage::TS))
            {
                shaders.push_back(requiredShaderList.at(ShaderStage::TS));
            }
            shaders.push_back(requiredShaderList.at(ShaderStage::MS));
            shaders.push_back(requiredShaderList.at(ShaderStage::FS));
        }
        // cs
        else if (requiredShaderList.contains(ShaderStage::CS))
        {
            shaders.push_back(requiredShaderList.at(ShaderStage::CS));
        }
        else
        {
            APH_ASSERT(false);
            return {Result::RuntimeError, "Unsupported shader stage combinations."};
        }

        ShaderReflector reflector{};
        ReflectRequest reflectRequest = {.shaders = shaders,
                                         .options = {.extractInputAttributes = true,
                                                     .extractOutputAttributes = true,
                                                     .extractPushConstants = true,
                                                     .extractSpecConstants = true,
                                                     .validateBindings = true,
                                                     .enableCaching = true,
                                                     .cachePath = generateReflectionCachePath(ppProgram, shaders)}};
        ReflectionResult reflectionResult = reflector.reflect(reflectRequest);

        vk::PipelineLayout* pipelineLayout = {};
        {
            // setup descriptor set layouts and pipeline layouts
            SmallVector<vk::DescriptorSetLayout*> setLayouts = {};

            // Get all active sets
            auto activeSets = ShaderReflector::getActiveDescriptorSets(reflectionResult);
            uint32_t numSets = activeSets.size();

            for (uint32_t i : activeSets)
            {
                vk::DescriptorSetLayoutCreateInfo setLayoutCreateInfo{
                    .bindings = ShaderReflector::getLayoutBindings(reflectionResult, i),
                    .poolSizes = ShaderReflector::getPoolSizes(reflectionResult, i),
                };
                vk::DescriptorSetLayout* layout = {};
                APH_VR(m_pDevice->create(setLayoutCreateInfo, &layout));
                setLayouts.push_back(layout);
            }

            if (auto maxBoundDescSets = m_pDevice->getPhysicalDevice()->getProperties().maxBoundDescriptorSets;
                numSets > maxBoundDescSets)
            {
                VK_LOG_ERR("Number of sets %u exceeds device limit of %u.", numSets, maxBoundDescSets);
            }

            vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{
                .vertexInput = reflectionResult.vertexInput,
                .pushConstantRange = reflectionResult.pushConstantRange,
                .setLayouts = std::move(setLayouts),
            };

            if (reflectionResult.pushConstantRange.stageFlags)
            {
                pipelineLayoutCreateInfo.pushConstantRange = reflectionResult.pushConstantRange;
            }

            APH_VR(m_pDevice->create(pipelineLayoutCreateInfo, &pipelineLayout));
        }
        vk::ProgramCreateInfo programCreateInfo{.shaders = std::move(requiredShaderList),
                                                .pPipelineLayout = pipelineLayout};
        APH_VR(m_pDevice->create(programCreateInfo, ppProgram));
    }

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
}

ShaderLoader::ShaderLoader(vk::Device* pDevice)
    : m_pDevice(pDevice)
    , m_pSlangLoaderImpl(std::make_unique<SlangLoaderImpl>())
{
    // Start the initialization task in the background
    auto& taskManager = APH_DEFAULT_TASK_MANAGER;
    auto taskGroup = taskManager.createTaskGroup("SlangInitialization");
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
