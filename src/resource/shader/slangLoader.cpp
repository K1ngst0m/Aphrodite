#include "slangLoader.h"
#include "common/profiler.h"
#include "filesystem/filesystem.h"
#include "global/globalManager.h"
#include "shaderCache.h"
#include "shaderUtil.h"

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

    slang::createGlobalSession(m_globalSession.writeRef());

    co_return Result::Success;
}

Result SlangLoaderImpl::createSlangSession(slang::ISession** ppOutSession)
{
    APH_PROFILER_SCOPE();

    if (!m_initialized.load() || !m_globalSession)
    {
        return {Result::RuntimeError, "SlangLoader not initialized"};
    }

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

    auto& fs             = APH_DEFAULT_FILESYSTEM;
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

    SlangResult result = m_globalSession->createSession(sessionDesc, ppOutSession);
    if (!SLANG_SUCCEEDED(result))
    {
        return {Result::RuntimeError, "Could not init slang session."};
    }

    return Result::Success;
}

Result SlangLoaderImpl::loadProgram(const CompileRequest& request, ShaderCache* pShaderCache,
                                    HashMap<aph::ShaderStage, SlangProgram>& spvCodeMap)
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
    const auto& filename     = request.filename;
    const auto& moduleMap    = request.moduleMap;
    const bool forceUncached = request.forceUncached;

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

    // Check cache - if pShaderCache is provided, use it
    std::string cacheFilePath;
    bool cacheExists = false;

    if (pShaderCache && !forceUncached)
    {
        cacheExists = pShaderCache->checkShaderCache(request, cacheFilePath);

        if (cacheExists)
        {
            if (pShaderCache->readShaderCache(cacheFilePath, spvCodeMap))
            {
                CM_LOG_INFO("Loaded shader from cache: %s", cacheFilePath.c_str());
                return Result::Success;
            }
        }
    }
    else if (forceUncached)
    {
        CM_LOG_INFO("Compiling shader from source (forceUncached): %s", std::string(filename).c_str());
    }
    else
    {
        // Fall back to old method if ShaderCache not provided
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
        std::string requestHash = request.getHash();
        cacheFilePath           = fs.resolvePath("shader_cache://" + requestHash + ".cache").value();
    }

    // Create slang session
    Slang::ComPtr<slang::ISession> session;
    APH_VERIFY_RESULT(createSlangSession(session.writeRef()));

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
            SlangResult result = module->getDefinedEntryPoint(i, entryPoint.writeRef());
            APH_ASSERT(SLANG_SUCCEEDED(result));

            componentsToLink.push_back(Slang::ComPtr<slang::IComponentType>(entryPoint.get()));
        }

        Slang::ComPtr<slang::IComponentType> composed;
        SlangResult result =
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
            SlangResult result =
                program->getEntryPointCode(entryPointIndex, 0, spirvCode.writeRef(), diagnostics.writeRef());
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

    if (!cacheExists && !forceUncached)
    {
        auto result = writeShaderCacheFile(cacheFilePath, spvCodeMap);
        if (!result.success())
        {
            CM_LOG_WARN("Failed to write shader cache: %s", result.error().toString().data());
        }
        else
        {
            CM_LOG_INFO("Successfully cached shader: %s", cacheFilePath.c_str());
        }
    }
    else if (forceUncached)
    {
        CM_LOG_INFO("Skipping shader cache writing due to forceUncached flag");
    }

    return Result::Success;
}
auto SlangLoaderImpl::isShaderCachingSupported() const -> bool
{
    return m_initialized.load();
}
} // namespace aph
