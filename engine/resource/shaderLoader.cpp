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

#define SLANG_CR(diagnostics)                                           \
    do                                                                  \
    {                                                                   \
        if (diagnostics)                                                \
        {                                                               \
            auto errlog = (const char*)diagnostics->getBufferPointer(); \
            CM_LOG_ERR("[slang diagnostics]: %s", errlog);              \
            APH_ASSERT(false);                                          \
            return {};                                                  \
        }                                                               \
    } while (0)

aph::HashMap<aph::ShaderStage, std::pair<std::string, std::vector<uint32_t>>> loadSlangFromFile(
    std::string_view filename)
{
    APH_PROFILER_SCOPE();
    // TODO multi global session in different threads
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock{ mtx };
    using namespace slang;
    static Slang::ComPtr<IGlobalSession> globalSession;
    {
        static std::once_flag flag;
        std::call_once(flag, []() { slang::createGlobalSession(globalSession.writeRef()); });
    }

    SlangResult result = {};
    Slang::ComPtr<IBlob> diagnostics;

    // create session
    Slang::ComPtr<ISession> session;
    {
        std::vector<CompilerOptionEntry> compilerOptions{{.name = CompilerOptionName::VulkanUseEntryPointName,
                                                        .value =
                                                            {
                                                                .kind      = CompilerOptionValueKind::Int,
                                                                .intValue0 = 1,
                                                            }},
                                                        {.name = CompilerOptionName::EmitSpirvMethod,
                                                        .value{
                                                            .kind      = CompilerOptionValueKind::Int,
                                                            .intValue0 = SLANG_EMIT_SPIRV_DIRECTLY,
                                                        }}};

        TargetDesc targetDesc;
        targetDesc.format = SLANG_SPIRV;
        targetDesc.profile = globalSession->findProfile("spirv_1_6");

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

        result = globalSession->createSession(sessionDesc, session.writeRef());
        APH_ASSERT(SLANG_SUCCEEDED(result));
    }

    // load program
    Slang::ComPtr<slang::IComponentType> program;
    {
        IModule* module = {};
        auto fname = aph::Filesystem::GetInstance().resolvePath(filename);
        module = session->loadModule(fname.c_str(), diagnostics.writeRef());
        SLANG_CR(diagnostics);

        std::vector<Slang::ComPtr<slang::IComponentType>> componentsToLink;
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

        result = composed->link(program.writeRef(), diagnostics.writeRef());
        SLANG_CR(diagnostics);
    }

    slang::ProgramLayout* programLayout = program->getLayout(0, diagnostics.writeRef());
    {
        SLANG_CR(diagnostics);

        if (!programLayout)
        {
            CM_LOG_ERR("Failed to get program layout");
            APH_ASSERT(false);
            return {};
        }
    }

    static const aph::HashMap<SlangStage, aph::ShaderStage> slangStageToShaderStageMap = {
        { SLANG_STAGE_VERTEX, aph::ShaderStage::VS },  { SLANG_STAGE_FRAGMENT, aph::ShaderStage::FS },
        { SLANG_STAGE_COMPUTE, aph::ShaderStage::CS }, { SLANG_STAGE_AMPLIFICATION, aph::ShaderStage::TS },
        { SLANG_STAGE_MESH, aph::ShaderStage::MS },
    };

    aph::HashMap<aph::ShaderStage, std::pair<std::string, std::vector<uint32_t>>> spvCodes;
    for (int entryPointIndex = 0; entryPointIndex < programLayout->getEntryPointCount(); entryPointIndex++)
    {
        EntryPointReflection* entryPointReflection = programLayout->getEntryPointByIndex(entryPointIndex);

        Slang::ComPtr<slang::IBlob> spirvCode;
        {
            result = program->getEntryPointCode(entryPointIndex, 0, spirvCode.writeRef(), diagnostics.writeRef());
            SLANG_CR(diagnostics);
            APH_ASSERT(SLANG_SUCCEEDED(result));
        }

        {
            std::vector<uint32_t> retSpvCode;
            retSpvCode.resize(spirvCode->getBufferSize() / sizeof(retSpvCode[0]));
            std::memcpy(retSpvCode.data(), spirvCode->getBufferPointer(), spirvCode->getBufferSize());

            std::string entryPointName = entryPointReflection->getName();
            aph::ShaderStage stage = slangStageToShaderStageMap.at(entryPointReflection->getStage());

            // TODO use the entrypoint name in loading info
            if (spvCodes.contains(stage))
            {
                CM_LOG_WARN("The shader file %s has mutliple entry point of [%s] stage. \
                            \nThe shader module would use the first one.",
                            filename, aph::vk::utils::toString(stage));
            }
            else
            {
                spvCodes[stage] = { entryPointName, std::move(retSpvCode) };
            }
        }
    }

    return spvCodes;
}

#undef SLANG_CR
} // namespace aph::loader::shader
