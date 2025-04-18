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
auto determinePipelineType(const HashMap<ShaderStage, vk::Shader*>& shaders) -> PipelineType;
auto orderShadersByPipeline(const HashMap<ShaderStage, vk::Shader*>& shaders, PipelineType pipelineType)
    -> SmallVector<vk::Shader*>;

// Cache utilities
auto generateReflectionCachePath(const SmallVector<vk::Shader*>& shaders) -> std::string;
auto generateCacheKey(const std::vector<std::string>& shaderPaths, const HashMap<ShaderStage, std::string>& stageInfo)
    -> std::string;

// Shader creation utilities
auto createShaderFromSPIRV(ThreadSafeObjectPool<vk::Shader>& shaderPool, const std::vector<uint32_t>& spirvCode,
                           ShaderStage stage, const std::string& entryPoint = "main") -> vk::Shader*;

// Write cache utility
auto writeShaderCacheFile(const std::string& cacheFilePath, const HashMap<ShaderStage, SlangProgram>& spvCodeMap)
    -> Expected<bool>;
} // namespace aph