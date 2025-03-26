#include "shaderLoader.h"
#include "common/profiler.h"
#include "filesystem/filesystem.h"

#include "slang-com-ptr.h"
#include "slang.h"

namespace aph::loader::shader
{
std::vector<uint32_t> loadSpvFromFile(std::string_view filename)
{
    APH_PROFILER_SCOPE();
    std::string source = Filesystem::GetInstance().readFileToString(filename);
    APH_ASSERT(!source.empty());
    uint32_t size = source.size();
    std::vector<uint32_t> spirv(size / sizeof(uint32_t));
    memcpy(spirv.data(), source.data(), size);
    return spirv;
}
} // namespace aph::loader::shader

namespace aph
{
#define SLANG_CR(diagnostics)                                           \
    do                                                                  \
    {                                                                   \
        if (diagnostics)                                                \
        {                                                               \
            auto errlog = (const char*)diagnostics->getBufferPointer(); \
            CM_LOG_ERR("[slang diagnostics]: %s", errlog);              \
            APH_ASSERT(false);                                          \
            return Result::RuntimeError;                                \
        }                                                               \
    } while (0)

using namespace slang;

struct SlangProgram
{
    std::string entryPoint;
    std::vector<uint32_t> spvCodes;
};

struct CompileRequest
{
    std::string_view filename;
    HashMap<std::string, std::string> moduleMap;

    template <typename T, typename U>
    void addModule(T&& name, U&& source)
    {
        moduleMap[std::forward<T>(name)] = std::forward<U>(source);
    }

    std::string getHash() const
    {
        APH_PROFILER_SCOPE();
        std::stringstream ss;
        ss << filename;

        // Sort moduleMap entries to ensure consistent hashing
        SmallVector<std::pair<std::string, std::string>> sortedModules(moduleMap.begin(), moduleMap.end());
        std::sort(sortedModules.begin(), sortedModules.end());

        for (const auto& [name, source] : sortedModules)
        {
            ss << name << source;
        }

        // Generate a simple hash from the content
        std::string content = ss.str();
        std::size_t hash = std::hash<std::string>{}(content);

        // Convert to hex string
        std::stringstream hexStream;
        hexStream << std::hex << std::setw(16) << std::setfill('0') << hash;

        return hexStream.str();
    }
};

class SlangLoaderImpl
{
public:
    SlangLoaderImpl()
    {
        APH_PROFILER_SCOPE();
        slang::createGlobalSession(m_globalSession.writeRef());
    }

    Result loadProgram(const CompileRequest& request, HashMap<aph::ShaderStage, SlangProgram>& spvCodeMap)
    {
        APH_PROFILER_SCOPE();
        static std::mutex fileWriterMtx;
        std::lock_guard<std::mutex> lock{ fileWriterMtx };
        const auto& filename = request.filename;
        const auto& moduleMap = request.moduleMap;

        auto& fs = aph::Filesystem::GetInstance();
        
        std::string cacheDirPath = fs.resolvePath("shader_cache://").string();
        if (!fs.exist(cacheDirPath))
        {
            if (!fs.createDirectories(cacheDirPath))
            {
                CM_LOG_WARN("Failed to create shader cache directory: %s", cacheDirPath.c_str());
                // Continue without caching
            }
        }

        // Generate a hash for the compile request
        std::string requestHash = request.getHash();
        std::string cacheFilePath = fs.resolvePath("shader_cache://" + requestHash + ".cache").string();

        // Check if the cache file exists
        if (fs.exist(cacheFilePath))
        {
            auto cacheBytes = fs.readFileToBytes(cacheFilePath);
            if (cacheBytes.size() > 0)
            {
                // Parse cache data
                size_t offset = 0;
                
                // Read number of shader stages
                if (offset + sizeof(uint32_t) <= cacheBytes.size())
                {
                    uint32_t numStages;
                    std::memcpy(&numStages, cacheBytes.data() + offset, sizeof(uint32_t));
                    offset += sizeof(uint32_t);
                    
                    bool cacheValid = true;
                    // Read each shader stage data
                    for (uint32_t i = 0; i < numStages && cacheValid; ++i)
                    {
                        // Read stage value and entry point length
                        if (offset + sizeof(uint32_t) * 2 > cacheBytes.size())
                        {
                            CM_LOG_WARN("Cache file corrupted: too small for stage header, file: %s", cacheFilePath.c_str());
                            cacheValid = false;
                            break;
                        }
                        
                        uint32_t stageVal;
                        std::memcpy(&stageVal, cacheBytes.data() + offset, sizeof(uint32_t));
                        offset += sizeof(uint32_t);
                        
                        uint32_t entryPointLength;
                        std::memcpy(&entryPointLength, cacheBytes.data() + offset, sizeof(uint32_t));
                        offset += sizeof(uint32_t);
                        
                        aph::ShaderStage stage = static_cast<aph::ShaderStage>(stageVal);
                        
                        // Read entry point
                        if (offset + entryPointLength > cacheBytes.size())
                        {
                            CM_LOG_WARN("Cache file corrupted: too small for entry point, file: %s", cacheFilePath.c_str());
                            cacheValid = false;
                            break;
                        }
                        std::string entryPoint(entryPointLength, '\0');
                        std::memcpy(entryPoint.data(), cacheBytes.data() + offset, entryPointLength);
                        offset += entryPointLength;
                        
                        // Read spv code length
                        if (offset + sizeof(uint32_t) > cacheBytes.size())
                        {
                            CM_LOG_WARN("Cache file corrupted: too small for code size, file: %s", cacheFilePath.c_str());
                            cacheValid = false;
                            break;
                        }
                        uint32_t codeSize;
                        std::memcpy(&codeSize, cacheBytes.data() + offset, sizeof(uint32_t));
                        offset += sizeof(uint32_t);
                        
                        // Read spv code
                        if (offset + codeSize > cacheBytes.size())
                        {
                            CM_LOG_WARN("Cache file corrupted: too small for SPIR-V code, file: %s", cacheFilePath.c_str());
                            cacheValid = false;
                            break;
                        }
                        std::vector<uint32_t> spvCode(codeSize / sizeof(uint32_t));
                        std::memcpy(spvCode.data(), cacheBytes.data() + offset, codeSize);
                        offset += codeSize;
                        
                        // Store in spvCodeMap
                        spvCodeMap[stage] = { entryPoint, std::move(spvCode) };
                    }
                    
                    if (cacheValid)
                    {
                        // Cache hit successful
                        return Result::Success;
                    }
                    else
                    {
                        // Clear any partial data
                        spvCodeMap.clear();
                    }
                }
                else
                {
                    CM_LOG_WARN("Cache file too small for header: %s", cacheFilePath.c_str());
                }
            }
            else
            {
                CM_LOG_WARN("Empty cache file: %s", cacheFilePath.c_str());
            }
        }

        // Cache miss or failed to read cache, compile the shader
        Slang::ComPtr<slang::ISession> session = {};
        SlangResult result = {};
        {
            std::vector<CompilerOptionEntry> compilerOptions{
            // TODO not working
            {.name = CompilerOptionName::DisableWarning, .value = {.kind      = CompilerOptionValueKind::String, .stringValue0 = "39001",}},
            {.name = CompilerOptionName::DisableWarning, .value = {.kind      = CompilerOptionValueKind::String, .stringValue0 = "parameterBindingsOverlap",}},
            {.name = CompilerOptionName::VulkanUseEntryPointName, .value = {.kind      = CompilerOptionValueKind::Int, .intValue0 = 1,}},
            {.name = CompilerOptionName::EmitSpirvMethod, .value{.kind      = CompilerOptionValueKind::Int, .intValue0 = SLANG_EMIT_SPIRV_DIRECTLY,}}};

            TargetDesc targetDesc;
            targetDesc.format = SLANG_SPIRV;
            targetDesc.profile = m_globalSession->findProfile("spirv_1_6");

            targetDesc.compilerOptionEntryCount = compilerOptions.size();
            targetDesc.compilerOptionEntries = compilerOptions.data();

            SessionDesc sessionDesc;
            sessionDesc.targets = &targetDesc;
            sessionDesc.targetCount = 1;

            auto shaderAssetPath = aph::Filesystem::GetInstance().resolvePath("shader_slang://");

            const std::array<const char*, 1> searchPaths{
                shaderAssetPath.c_str(),
            };

            sessionDesc.searchPaths = searchPaths.data();
            sessionDesc.searchPathCount = searchPaths.size();

            // PreprocessorMacroDesc fancyFlag = { "ENABLE_FANCY_FEATURE", "1" };
            // sessionDesc.preprocessorMacros = &fancyFlag;
            // sessionDesc.preprocessorMacroCount = 1;

            result = m_globalSession->createSession(sessionDesc, session.writeRef());
            if (!SLANG_SUCCEEDED(result))
            {
                return { Result::RuntimeError, "Could not init slang session." };
            }
        }

        Slang::ComPtr<IBlob> diagnostics;

        Slang::ComPtr<slang::IComponentType> program;
        {
            APH_PROFILER_SCOPE();
            IModule* module = {};
            auto fname = aph::Filesystem::GetInstance().resolvePath(filename);

            std::vector<Slang::ComPtr<slang::IComponentType>> componentsToLink;
            std::string patchCode;
            {
                APH_PROFILER_SCOPE_NAME("load module from string");
                std::stringstream ss;
                for (const auto& [name, src] : moduleMap)
                {
                    ss << std::format("import {};\n", name);
                    {
                        APH_PROFILER_SCOPE_NAME("load patch module");
                        auto m =
                            session->loadModuleFromSourceString(name.c_str(), "", src.c_str(), diagnostics.writeRef());
                        componentsToLink.push_back(Slang::ComPtr<slang::IComponentType>(m));
                    }
                }
                patchCode = ss.str();

                auto shaderSource = patchCode + aph::Filesystem::GetInstance().readFileToString(filename);
                {
                    APH_PROFILER_SCOPE_NAME("load main module");
                    module = session->loadModuleFromSourceString("hello_mesh_bindless", fname.c_str(),
                                                                 shaderSource.c_str(), diagnostics.writeRef());
                }
            }

            SLANG_CR(diagnostics);

            for (int i = 0; i < module->getDefinedEntryPointCount(); i++)
            {
                Slang::ComPtr<slang::IEntryPoint> entryPoint;
                result = module->getDefinedEntryPoint(i, entryPoint.writeRef());
                APH_ASSERT(SLANG_SUCCEEDED(result));

                componentsToLink.push_back(Slang::ComPtr<slang::IComponentType>(entryPoint.get()));
            }

            Slang::ComPtr<slang::IComponentType> composed;
            result = session->createCompositeComponentType((slang::IComponentType**)componentsToLink.data(),
                                                           componentsToLink.size(), composed.writeRef(),
                                                           diagnostics.writeRef());
            APH_ASSERT(SLANG_SUCCEEDED(result));

            {
                APH_PROFILER_SCOPE_NAME("link program");
                result = composed->link(program.writeRef(), diagnostics.writeRef());
                SLANG_CR(diagnostics);
            }
        }

        slang::ProgramLayout* programLayout = program->getLayout(0, diagnostics.writeRef());
        {
            SLANG_CR(diagnostics);

            if (!programLayout)
            {
                APH_ASSERT(false);
                return { Result::RuntimeError, "Failed to get program layout" };
            }
        }

        static const aph::HashMap<SlangStage, aph::ShaderStage> slangStageToShaderStageMap = {
            { SLANG_STAGE_VERTEX, aph::ShaderStage::VS },  { SLANG_STAGE_FRAGMENT, aph::ShaderStage::FS },
            { SLANG_STAGE_COMPUTE, aph::ShaderStage::CS }, { SLANG_STAGE_AMPLIFICATION, aph::ShaderStage::TS },
            { SLANG_STAGE_MESH, aph::ShaderStage::MS },
        };

        for (int entryPointIndex = 0; entryPointIndex < programLayout->getEntryPointCount(); entryPointIndex++)
        {
            APH_PROFILER_SCOPE();
            EntryPointReflection* entryPointReflection = programLayout->getEntryPointByIndex(entryPointIndex);

            Slang::ComPtr<slang::IBlob> spirvCode;
            {
                result = program->getEntryPointCode(entryPointIndex, 0, spirvCode.writeRef(), diagnostics.writeRef());
                SLANG_CR(diagnostics);
                APH_ASSERT(SLANG_SUCCEEDED(result));
            }

            {
                APH_PROFILER_SCOPE_NAME("get spirv code");
                std::vector<uint32_t> retSpvCode;
                retSpvCode.resize(spirvCode->getBufferSize() / sizeof(retSpvCode[0]));
                std::memcpy(retSpvCode.data(), spirvCode->getBufferPointer(), spirvCode->getBufferSize());

                std::string entryPointName = entryPointReflection->getName();
                aph::ShaderStage stage = slangStageToShaderStageMap.at(entryPointReflection->getStage());

                // TODO use the entrypoint name in loading info
                if (spvCodeMap.contains(stage))
                {
                    CM_LOG_WARN("The shader file %s has mutliple entry point of [%s] stage. \
                                \nThe shader module would use the first one.",
                                filename, aph::vk::utils::toString(stage));
                }
                else
                {
                    spvCodeMap[stage] = { entryPointName, std::move(retSpvCode) };
                }
            }
        }

        // Save to cache
        // Prepare the cache data
        std::vector<uint8_t> cacheData;
        
        // Reserve some initial space
        cacheData.reserve(1024 * 1024); // 1MB initial reservation
        
        // Write header (number of shader stages)
        uint32_t numStages = static_cast<uint32_t>(spvCodeMap.size());
        size_t headerSize = sizeof(uint32_t);
        cacheData.resize(headerSize);
        std::memcpy(cacheData.data(), &numStages, headerSize);
        
        // Write each shader stage data
        for (const auto& [stage, slangProgram] : spvCodeMap)
        {
            // Write stage header (stage value and entry point length)
            uint32_t stageVal = static_cast<uint32_t>(stage);
            uint32_t entryPointLength = static_cast<uint32_t>(slangProgram.entryPoint.size());
            
            size_t stageHeaderSize = sizeof(uint32_t) * 2;
            size_t stageHeaderOffset = cacheData.size();
            cacheData.resize(stageHeaderOffset + stageHeaderSize);
            std::memcpy(cacheData.data() + stageHeaderOffset, &stageVal, sizeof(uint32_t));
            std::memcpy(cacheData.data() + stageHeaderOffset + sizeof(uint32_t), &entryPointLength, sizeof(uint32_t));
            
            // Write entry point
            size_t entryPointOffset = cacheData.size();
            cacheData.resize(entryPointOffset + entryPointLength);
            if (entryPointLength > 0) {
                std::memcpy(cacheData.data() + entryPointOffset, slangProgram.entryPoint.data(), entryPointLength);
            }
            
            // Write spv code length
            uint32_t codeSize = static_cast<uint32_t>(slangProgram.spvCodes.size() * sizeof(uint32_t));
            size_t codeSizeOffset = cacheData.size();
            cacheData.resize(codeSizeOffset + sizeof(uint32_t));
            std::memcpy(cacheData.data() + codeSizeOffset, &codeSize, sizeof(uint32_t));
            
            // Write spv code
            size_t codeOffset = cacheData.size();
            cacheData.resize(codeOffset + codeSize);
            if (codeSize > 0) {
                std::memcpy(cacheData.data() + codeOffset, slangProgram.spvCodes.data(), codeSize);
            }
        }
        
        // Write the cache file directly to avoid exception handling
        // This duplicates the logic from Filesystem::writeBytesToFile but without exceptions
        std::ofstream file(fs.resolvePath(cacheFilePath).string(), std::ios::binary);
        if (!file)
        {
            CM_LOG_WARN("Failed to open cache file for writing: %s", cacheFilePath.c_str());
        }
        else
        {
            file.write(reinterpret_cast<const char*>(cacheData.data()), cacheData.size());
            if (!file.good())
            {
                CM_LOG_WARN("Failed to write shader cache for %s", filename);
            }
        }

        return Result::Success;
    }

private:
    Slang::ComPtr<slang::IGlobalSession> m_globalSession = {};
};
} // namespace aph

namespace aph
{
Result ShaderLoader::load(const ShaderLoadInfo& info, vk::ShaderProgram** ppProgram)
{
    APH_PROFILER_SCOPE();
    CompileRequest compileRequest{};
    if (info.pBindlessResource)
    {
        // TODO unused since the warning suppress compiler option not working
        compileRequest.addModule(
            "bindless", aph::Filesystem::GetInstance().readFileToString("shader_slang://modules/bindless.slang"));
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
            std::unique_lock<std::mutex> lock{ m_loadMtx };

            if (auto it = m_shaderCaches.find(d); it == m_shaderCaches.end())
            {
                std::promise<ShaderCacheData> promise;
                future = promise.get_future().share();
                m_shaderCaches[d] = future;
                lock.unlock();

                {
                    HashMap<ShaderStage, SlangProgram> spvCodeMap;
                    auto path = Filesystem::GetInstance().resolvePath(d);
                    compileRequest.filename = path.c_str();
                    APH_VR(m_pSlangLoaderImpl->loadProgram(compileRequest, spvCodeMap));
                    if (spvCodeMap.empty())
                    {
                        return { Result::RuntimeError, "Failed to load slang shader from file." };
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

    APH_VR(m_pDevice->create(vk::ProgramCreateInfo{ .shaders = std::move(requiredShaderList) }, ppProgram));

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
}
} // namespace aph
