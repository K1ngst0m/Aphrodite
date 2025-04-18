#pragma once

#include "api/vulkan/vkUtils.h"
#include "common/hash.h"
#include "common/smallVector.h"
#include "shaderLoader.h"
#include "slang-com-ptr.h"
#include "slang.h"
#include "threads/taskManager.h"

namespace aph
{
struct SlangProgram
{
    std::string entryPoint;
    std::vector<uint32_t> spvCodes;
};

class ShaderCache;

class SlangLoaderImpl
{
public:
    SlangLoaderImpl();

    auto initialize() -> TaskType;

    auto isShaderCachingSupported() const -> bool;

    auto loadProgram(const CompileRequest& request, ShaderCache* pShaderCache,
                     HashMap<aph::ShaderStage, SlangProgram>& spvCodeMap) -> Result;

private:
    auto createSlangSession(slang::ISession** ppOutSession) -> Result;

    Slang::ComPtr<slang::IGlobalSession> m_globalSession = {};
    std::atomic<bool> m_initialized                      = false;
};
} // namespace aph
