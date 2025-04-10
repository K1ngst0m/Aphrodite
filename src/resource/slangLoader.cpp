#include "slangLoader.h"
#include "common/profiler.h"
#include "filesystem/filesystem.h"
#include "global/globalManager.h"

namespace aph
{

using namespace slang;

std::string CompileRequest::getHash() const
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
    std::size_t hash    = std::hash<std::string>{}(content);

    // Convert to hex string
    std::stringstream hexStream;
    hexStream << std::hex << std::setw(16) << std::setfill('0') << hash;

    return hexStream.str();
}
} // namespace aph

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

SlangLoaderImpl::SlangLoaderImpl()
{
    APH_PROFILER_SCOPE();
    // We'll initialize the global session asynchronously
    m_initialized = false;
}
TaskType SlangLoaderImpl::initialize()
{
    APH_PROFILER_SCOPE();
    // Only initialize once
    if (m_initialized.exchange(true))
    {
        co_return Result::Success;
    }

    // The expensive operation will be done in a separate thread
    slang::createGlobalSession(m_globalSession.writeRef());

    co_return Result::Success;
}
bool SlangLoaderImpl::checkShaderCache(const CompileRequest& request, std::string& outCachePath)
{
    APH_PROFILER_SCOPE();
    auto& fs = APH_DEFAULT_FILESYSTEM;

    // First check if the shader_cache protocol is registered
    std::string cacheDirPath = fs.resolvePath("shader_cache://").value();
    if (!fs.exist(cacheDirPath))
    {
        return false;
    }

    // Generate hash and check if cache file exists
    std::string requestHash = request.getHash();
    outCachePath            = fs.resolvePath("shader_cache://" + requestHash + ".cache").value();
    return fs.exist(outCachePath);
}
bool SlangLoaderImpl::readShaderCache(const std::string& cacheFilePath,
                                      HashMap<aph::ShaderStage, SlangProgram>& spvCodeMap)
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
Result SlangLoaderImpl::loadProgram(const CompileRequest& request, HashMap<aph::ShaderStage, SlangProgram>& spvCodeMap)
{
    APH_PROFILER_SCOPE();
    // Make sure initialization is complete before proceeding
    if (!m_initialized.load())
    {
        CM_LOG_ERR("SlangLoaderImpl not initialized before use");
        return Result::RuntimeError;
    }

    static std::mutex fileWriterMtx;
    std::lock_guard<std::mutex> lock{fileWriterMtx};
    const auto& filename  = request.filename;
    const auto& moduleMap = request.moduleMap;

    auto& fs = APH_DEFAULT_FILESYSTEM;

    // If slangDumpPath is provided, ensure the directory exists
    bool canDumpSlang = false;
    if (!request.slangDumpPath.empty())
    {
        // Treat slangDumpPath as a directory
        std::filesystem::path slangDumpDir = request.slangDumpPath;

        // Check if path exists
        if (fs.exist(slangDumpDir.string()))
        {
            // Check if it's a directory or a file
            if (std::filesystem::is_directory(slangDumpDir))
            {
                CM_LOG_INFO("Using existing Slang dump directory: %s", slangDumpDir.string().c_str());
                canDumpSlang = true;
            }
            else
            {
                // It's a file - not valid for our use
                CM_LOG_WARN("Slang dump path exists but is not a directory: %s. Shader source dumping disabled.",
                            slangDumpDir.string().c_str());
                canDumpSlang = false;
            }
        }
        else
        {
            // Doesn't exist - create it
            if (fs.createDirectories(slangDumpDir.string()))
            {
                CM_LOG_INFO("Created Slang dump directory: %s", slangDumpDir.string().c_str());
                canDumpSlang = true;
            }
            else
            {
                CM_LOG_WARN("Failed to create Slang dump directory: %s. Shader source dumping disabled.",
                            slangDumpDir.string().c_str());
                canDumpSlang = false;
            }
        }
    }

    std::string cacheDirPath = fs.resolvePath("shader_cache://");
    if (!fs.exist(cacheDirPath))
    {
        if (!fs.createDirectories(cacheDirPath))
        {
            CM_LOG_WARN("Failed to create shader cache directory: %s", cacheDirPath.c_str());
            // Continue without caching
        }
    }

    // Generate a hash for the compile request
    std::string requestHash   = request.getHash();
    std::string cacheFilePath = fs.resolvePath("shader_cache://" + requestHash + ".cache");

    // Skip cache checking - ShaderLoader already checked the cache
    // Proceed directly to compilation

    Slang::ComPtr<slang::ISession> session = {};
    SlangResult result                     = {};
    {
        std::vector<CompilerOptionEntry> compilerOptions{
            // TODO not working
            {         .name = CompilerOptionName::DisableWarning,
             .value =
             {
             .kind         = CompilerOptionValueKind::String,
             .stringValue0 = "39001",
             }},
            {         .name = CompilerOptionName::DisableWarning,
             .value =
             {
             .kind         = CompilerOptionValueKind::String,
             .stringValue0 = "parameterBindingsOverlap",
             }},
            {.name = CompilerOptionName::VulkanUseEntryPointName,
             .value =
             {
             .kind      = CompilerOptionValueKind::Int,
             .intValue0 = 1,
             }},
            {        .name = CompilerOptionName::EmitSpirvMethod,
             .value{
             .kind      = CompilerOptionValueKind::Int,
             .intValue0 = SLANG_EMIT_SPIRV_DIRECTLY,
             }}
        };

        TargetDesc targetDesc;
        targetDesc.format  = SLANG_SPIRV;
        targetDesc.profile = m_globalSession->findProfile("spirv_1_6");

        targetDesc.compilerOptionEntryCount = compilerOptions.size();
        targetDesc.compilerOptionEntries    = compilerOptions.data();

        SessionDesc sessionDesc;
        sessionDesc.targets     = &targetDesc;
        sessionDesc.targetCount = 1;

        auto shaderAssetPath = fs.resolvePath("shader_slang://");
        if (!shaderAssetPath.success())
        {
            CM_LOG_ERR("Failed to resolve shader_slang:// protocol");
            return {Result::RuntimeError, "Failed to resolve shader asset path"};
        }

        const std::array<const char*, 1> searchPaths{
            shaderAssetPath.value().c_str(),
        };

        sessionDesc.searchPaths     = searchPaths.data();
        sessionDesc.searchPathCount = searchPaths.size();

        // PreprocessorMacroDesc fancyFlag = { "ENABLE_FANCY_FEATURE", "1" };
        // sessionDesc.preprocessorMacros = &fancyFlag;
        // sessionDesc.preprocessorMacroCount = 1;

        result = m_globalSession->createSession(sessionDesc, session.writeRef());
        if (!SLANG_SUCCEEDED(result))
        {
            return {Result::RuntimeError, "Could not init slang session."};
        }
    }

    Slang::ComPtr<IBlob> diagnostics;

    Slang::ComPtr<slang::IComponentType> program;
    {
        APH_PROFILER_SCOPE();
        IModule* module = {};
        auto fname      = fs.resolvePath(filename);
        if (!fname.success())
        {
            CM_LOG_ERR("Failed to resolve shader path: %s", filename);
            return {Result::RuntimeError, "Failed to resolve shader path"};
        }

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
                    auto m = session->loadModuleFromSourceString(name.c_str(), "", src.c_str(), diagnostics.writeRef());
                    componentsToLink.push_back(Slang::ComPtr<slang::IComponentType>(m));
                }
            }
            patchCode = ss.str();

            auto shaderSource = patchCode + fs.readFileToString(filename).value();

            // Dump modules to slangDumpPath directory if requested
            if (canDumpSlang)
            {
                std::filesystem::path slangDumpDir = request.slangDumpPath;
                std::filesystem::path mainFilePath = filename;
                std::string mainFileName           = mainFilePath.filename().string();

                // Dump each module to a separate file
                for (const auto& [name, src] : moduleMap)
                {
                    std::string moduleFilePath = (slangDumpDir / (name + ".slang")).string();
                    auto writeResult           = fs.writeStringToFile(moduleFilePath, src);
                    if (!writeResult.success())
                    {
                        CM_LOG_WARN("Failed to dump module %s: %s", name.c_str(), writeResult.toString().data());
                    }
                    else
                    {
                        CM_LOG_INFO("Dumped module %s to %s", name.c_str(), moduleFilePath.c_str());
                    }
                }

                // Dump the main source file
                // std::string mainSourcePath = (slangDumpDir / mainFileName).string();
                // auto writeMainResult = fs.writeStringToFile(mainSourcePath, fs.readFileToString(filename).value());
                // if (writeMainResult.success())
                // {
                //     CM_LOG_INFO("Dumped main source to %s", mainSourcePath.c_str());
                // }

                // Dump the patched source (combined)
                std::string patchedFilePath = (slangDumpDir / ("patched_" + mainFileName)).string();
                auto writePatchedResult     = fs.writeStringToFile(patchedFilePath, shaderSource);
                if (!writePatchedResult.success())
                {
                    CM_LOG_WARN("Failed to dump patched source: %s", writePatchedResult.toString().data());
                }
                else
                {
                    CM_LOG_INFO("Dumped patched source to %s", patchedFilePath.c_str());
                }
            }

            {
                APH_PROFILER_SCOPE_NAME("load main module");
                module = session->loadModuleFromSourceString("hello_mesh_bindless", fname.value().c_str(),
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
        result =
            session->createCompositeComponentType((slang::IComponentType**)componentsToLink.data(),
                                                  componentsToLink.size(), composed.writeRef(), diagnostics.writeRef());
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
            return {Result::RuntimeError, "Failed to get program layout"};
        }
    }

    static const aph::HashMap<SlangStage, aph::ShaderStage> slangStageToShaderStageMap = {
        {       SLANG_STAGE_VERTEX, aph::ShaderStage::VS},
        {     SLANG_STAGE_FRAGMENT, aph::ShaderStage::FS},
        {      SLANG_STAGE_COMPUTE, aph::ShaderStage::CS},
        {SLANG_STAGE_AMPLIFICATION, aph::ShaderStage::TS},
        {         SLANG_STAGE_MESH, aph::ShaderStage::MS},
    };

    // If spvDumpPath is provided, prepare directory
    bool canDumpSpv = false;
    std::filesystem::path spvDumpDir;
    if (!request.spvDumpPath.empty())
    {
        spvDumpDir = std::filesystem::path(request.spvDumpPath).parent_path();

        // Check if path exists
        if (!spvDumpDir.empty())
        {
            if (fs.exist(spvDumpDir.string()))
            {
                // Check if it's a directory or a file
                if (std::filesystem::is_directory(spvDumpDir))
                {
                    CM_LOG_INFO("Using existing SPIR-V dump directory: %s", spvDumpDir.string().c_str());
                    canDumpSpv = true;
                }
                else
                {
                    // It's a file - not valid for our use
                    CM_LOG_WARN("SPIR-V dump parent path exists but is not a directory: %s. SPIR-V dumping disabled.",
                                spvDumpDir.string().c_str());
                    canDumpSpv = false;
                }
            }
            else
            {
                // Doesn't exist - create it
                if (fs.createDirectories(spvDumpDir.string()))
                {
                    CM_LOG_INFO("Created SPIR-V dump directory: %s", spvDumpDir.string().c_str());
                    canDumpSpv = true;
                }
                else
                {
                    CM_LOG_WARN("Failed to create SPIR-V dump directory: %s. SPIR-V dumping disabled.",
                                spvDumpDir.string().c_str());
                    canDumpSpv = false;
                }
            }
        }
        else
        {
            // No parent path (e.g., just a filename)
            canDumpSpv = true;
        }
    }

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
            aph::ShaderStage stage     = slangStageToShaderStageMap.at(entryPointReflection->getStage());

            // Dump the SPIR-V code if requested
            if (canDumpSpv)
            {
                // Create a unique filename based on the entry point and stage
                std::string stageName   = aph::vk::utils::toString(stage);
                std::string spvFilename = std::filesystem::path(request.spvDumpPath).stem().string() + "_" + stageName +
                                          "_" + entryPointName + ".spv";
                std::string spvFilePath = (spvDumpDir / spvFilename).string();

                // Write the SPIR-V binary file
                auto writeResult = fs.writeBinaryData(spvFilePath, retSpvCode.data(), retSpvCode.size());
                if (!writeResult.success())
                {
                    CM_LOG_WARN("Failed to write SPIR-V code: %s", writeResult.toString().data());
                }
                else
                {
                    CM_LOG_INFO("Dumped SPIR-V code for %s:%s to %s", stageName.c_str(), entryPointName.c_str(),
                                spvFilePath.c_str());
                }
            }

            // TODO use the entrypoint name in loading info
            if (spvCodeMap.contains(stage))
            {
                CM_LOG_WARN("The shader file %s has mutliple entry point of [%s] stage. \
                                \nThe shader module would use the first one.",
                            filename, aph::vk::utils::toString(stage));
            }
            else
            {
                spvCodeMap[stage] = {entryPointName, std::move(retSpvCode)};
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

    // Write the cache file directly to avoid exception handling
    // This duplicates the logic from Filesystem::writeBytesToFile but without exceptions
    std::ofstream file(fs.resolvePath(cacheFilePath).value(), std::ios::binary);
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
} // namespace aph
