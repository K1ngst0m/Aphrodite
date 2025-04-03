#pragma once

#include "api/vulkan/vkUtils.h"
#include "common/hash.h"
#include "common/smallVector.h"
#include "threads/taskManager.h"
#include "slang-com-ptr.h"
#include "slang.h"

namespace aph
{
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
    void addModule(T&& name, U&& source);

    std::string getHash() const;
};

class SlangLoaderImpl
{
public:
    SlangLoaderImpl();

    TaskType initialize();

    // Add a method to check if the cache exists without requiring initialization
    bool checkShaderCache(const CompileRequest& request, std::string& outCachePath);

    // Helper to read shader cache data
    bool readShaderCache(const std::string& cacheFilePath, HashMap<aph::ShaderStage, SlangProgram>& spvCodeMap);

    Result loadProgram(const CompileRequest& request, HashMap<aph::ShaderStage, SlangProgram>& spvCodeMap);

private:
    Slang::ComPtr<slang::IGlobalSession> m_globalSession = {};
    std::atomic<bool> m_initialized = false;
};

template <typename T, typename U>
inline void CompileRequest::addModule(T&& name, U&& source)
{
    moduleMap[std::forward<T>(name)] = std::forward<U>(source);
}
} // namespace aph
