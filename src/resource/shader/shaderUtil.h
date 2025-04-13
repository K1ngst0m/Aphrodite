#pragma once

#include "allocator/objectPool.h"
#include "api/vulkan/vkUtils.h"
#include "common/hash.h"
#include "common/result.h"
#include "resource/forward.h"
#include "shaderAsset.h"
#include "shaderLoader.h"

namespace aph
{
// Forward declarations
struct SlangProgram;
namespace vk
{
class Device;
class ShaderProgram;
class Shader;
} // namespace vk

// Shader type utilities
PipelineType determinePipelineType(const HashMap<ShaderStage, vk::Shader*>& shaders);
SmallVector<vk::Shader*> orderShadersByPipeline(const HashMap<ShaderStage, vk::Shader*>& shaders,
                                                PipelineType pipelineType);

// Cache utilities
std::string generateReflectionCachePath(const SmallVector<vk::Shader*>& shaders);
std::string generateCacheKey(const std::vector<std::string>& shaderPaths,
                             const HashMap<ShaderStage, std::string>& stageInfo);

// Shader creation utilities
vk::Shader* createShaderFromSPIRV(ThreadSafeObjectPool<vk::Shader>& shaderPool, const std::vector<uint32_t>& spirvCode,
                                  ShaderStage stage, const std::string& entryPoint = "main");

// Write cache utility
Expected<bool> writeShaderCacheFile(const std::string& cacheFilePath,
                                    const HashMap<ShaderStage, SlangProgram>& spvCodeMap);

} // namespace aph