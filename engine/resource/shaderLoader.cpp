#include "shaderLoader.h"
#include "common/profiler.h"
#include "filesystem/filesystem.h"

#include "slang-com-ptr.h"
#include "slang.h"

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

class SlangLoaderImpl
{
public:
    SlangLoaderImpl()
    {
        slang::createGlobalSession(m_globalSession.writeRef());
    }

    Result loadProgram(std::string_view filename, HashMap<aph::ShaderStage, SlangProgram>& spvCodeMap)
    {
        if (!m_session)
        {
            APH_VR(initSession());
        }

        SlangResult result = {};
        Slang::ComPtr<IBlob> diagnostics;

        Slang::ComPtr<slang::IComponentType> program;
        {
            IModule* module = {};
            auto fname = aph::Filesystem::GetInstance().resolvePath(filename);
            module = m_session->loadModule(fname.c_str(), diagnostics.writeRef());
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
            result = m_session->createCompositeComponentType((slang::IComponentType**)componentsToLink.data(),
                                                             componentsToLink.size(), composed.writeRef(),
                                                             diagnostics.writeRef());
            APH_ASSERT(SLANG_SUCCEEDED(result));

            result = composed->link(program.writeRef(), diagnostics.writeRef());
            SLANG_CR(diagnostics);
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

        return Result::Success;
    }

private:
    Result initSession()
    {
        SlangResult result = {};
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

            result = m_globalSession->createSession(sessionDesc, m_session.writeRef());
            if (!SLANG_SUCCEEDED(result))
            {
                return { Result::RuntimeError, "Could not init slang session." };
            }
        }

        return Result::Success;
    }

    Slang::ComPtr<slang::IGlobalSession> m_globalSession = {};
    Slang::ComPtr<slang::ISession> m_session = {};
};
} // namespace aph

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
Result ShaderLoader::load(vk::ShaderProgram** ppProgram)
{
    const auto& info = m_loadInfo;

    if (info.pBindlessResource)
    {
        const auto& fs = Filesystem::GetInstance();
        static std::mutex fileWriterMtx;
        std::lock_guard<std::mutex> lock{ fileWriterMtx };

        std::string genDir = "shader_slang://gen";
        if (!fs.exist(genDir))
        {
            std::filesystem::create_directory(fs.resolvePath(genDir));
        }
        // TODO
        auto genFile = genDir + "/" + "hello_mesh_bindless.slang";
        Filesystem::GetInstance().writeStringToFile(genFile, info.pBindlessResource->generateHandleSource());
    }

    auto loadShader = [this](const std::vector<uint32_t>& spv, const ShaderStage stage,
                             const std::string& entryPoint = "main") -> vk::Shader*
    {
        vk::Shader* shader;
        vk::ShaderCreateInfo createInfo{
            .code = spv,
            .entrypoint = entryPoint,
            .stage = stage,
        };
        APH_VR(m_pDevice->create(createInfo, &shader));
        return shader;
    };

    HashMap<ShaderStage, vk::Shader*> requiredShaderList;
    for (const auto& d : info.data)
    {
        HashMap<ShaderStage, SlangProgram> spvCodeMap;
        auto path = Filesystem::GetInstance().resolvePath(d);

        APH_VR(m_pSlangLoaderImpl->loadProgram(path.c_str(), spvCodeMap));
        if (spvCodeMap.empty())
        {
            return { Result::RuntimeError, "Failed to load slang shader from file." };
        }

        for (const auto& [stage, entryPoint] : info.stageInfo)
        {
            APH_ASSERT(spvCodeMap.contains(stage) && spvCodeMap.at(stage).entryPoint == entryPoint);
            const auto& spv = spvCodeMap.at(stage).spvCodes;
            vk::Shader* shader = loadShader(spv, stage, entryPoint);
            requiredShaderList[stage] = shader;
        }
    }

    APH_VR(m_pDevice->create(vk::ProgramCreateInfo{ .shaders = std::move(requiredShaderList) }, ppProgram));

    return Result::Success;
}

ShaderLoader::~ShaderLoader()
{
}

ShaderLoader::ShaderLoader(vk::Device* pDevice, ShaderLoadInfo loadInfo)
    : m_loadInfo(std::move(loadInfo))
    , m_pDevice(pDevice)
{
    m_pSlangLoaderImpl = std::make_unique<SlangLoaderImpl>();
}
} // namespace aph
