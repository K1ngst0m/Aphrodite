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

    TaskType initialize();

    // Check if we support loading a shader from cache
    bool isShaderCachingSupported() const
    {
        return m_initialized.load();
    }

    Result loadProgram(const CompileRequest& request, ShaderCache* pShaderCache,
                       HashMap<aph::ShaderStage, SlangProgram>& spvCodeMap);

private:
    Result createSlangSession(slang::ISession** ppOutSession);

    Slang::ComPtr<slang::IGlobalSession> m_globalSession = {};
    std::atomic<bool> m_initialized                      = false;
};
} // namespace aph
