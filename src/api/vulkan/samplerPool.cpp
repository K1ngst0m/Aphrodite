#include "samplerPool.h"
#include "common/profiler.h"
#include "device.h"

namespace aph::vk
{
constexpr auto toString(PresetSamplerType type) -> std::string_view
{
    switch (type)
    {
    case PresetSamplerType::eLinearClampMipmap:
        return "LinearClampMipmap";
    case PresetSamplerType::eLinearWrapMipmap:
        return "LinearWrapMipmap";
    case PresetSamplerType::eLinearMirrorMipmap:
        return "LinearMirrorMipmap";
    case PresetSamplerType::eNearestClampMipmap:
        return "NearestClampMipmap";
    case PresetSamplerType::eNearestWrapMipmap:
        return "NearestWrapMipmap";
    case PresetSamplerType::eAnisotropicClamp:
        return "AnisotropicClamp";
    case PresetSamplerType::eAnisotropicWrap:
        return "AnisotropicWrap";
    case PresetSamplerType::eShadowPcf:
        return "ShadowPCF";
    case PresetSamplerType::eShadowEsm:
        return "ShadowESM";
    case PresetSamplerType::eCubemap:
        return "Cubemap";
    case PresetSamplerType::eCubemapLow:
        return "CubemapLow";
    case PresetSamplerType::ePointClamp:
        return "PointClamp";
    default:
        return "Unknown";
    }
}

SamplerPool::SamplerPool(Device* pDevice)
    : m_pDevice(pDevice)
{
    APH_ASSERT(pDevice, "Device cannot be null");
}

SamplerPool::~SamplerPool()
{
    for (auto& sampler : m_samplers)
    {
        if (sampler != nullptr)
        {
            m_pDevice->destroy(sampler);
            sampler = nullptr;
        }
    }
}

auto SamplerPool::initialize() -> Result
{
    APH_PROFILER_SCOPE();

    // Create all predefined samplers
    for (size_t i = 0; i < kSamplerTypeCount; ++i)
    {
        auto result = createPredefinedSampler(static_cast<PresetSamplerType>(i));
        if (!result.success())
        {
            CM_LOG_ERR("Failed to create sampler type %s: %s", toString(static_cast<PresetSamplerType>(i)).data(),
                       result.toString());
            return result;
        }
    }

    return Result::Success;
}

auto SamplerPool::createPredefinedSampler(PresetSamplerType type) -> Result
{
    APH_PROFILER_SCOPE();

    std::lock_guard<std::mutex> lock(m_mutex);

    // Get creation info for this type
    SamplerCreateInfo createInfo = getCreateInfoFromType(type);

    // Create the sampler using Device's createImpl with isPoolInitialization=true
    // to avoid the circular dependency during initialization
    std::string debugName = std::string("PoolSampler_") + std::string(toString(type));

    // Use the device's createImpl method directly with isPoolInitialization=true
    auto samplerResult = m_pDevice->createImpl(createInfo, true);

    if (!samplerResult.success())
    {
        return samplerResult;
    }

    // Set debug name manually since we bypassed the normal create method
    if (!debugName.empty())
    {
        auto result = m_pDevice->setDebugObjectName(samplerResult.value(), debugName);
        if (!result.success())
        {
            CM_LOG_WARN("Failed to set debug name for sampler %s: %s", toString(type).data(), result.toString());
            // Continue despite name setting failure
        }
    }

    // Store in the pool
    m_samplers[static_cast<size_t>(type)] = samplerResult.value();

    return Result::Success;
}

auto SamplerPool::getSampler(PresetSamplerType type) const -> Sampler*
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (type >= PresetSamplerType::eCount)
    {
        CM_LOG_WARN("Requested invalid sampler type %s", toString(type).data());
        return nullptr;
    }

    return m_samplers[static_cast<size_t>(type)];
}

auto SamplerPool::findMatchingSampler(const SamplerCreateInfo& config) const -> Sampler*
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Check all predefined samplers for a match
    for (auto* sampler : m_samplers)
    {
        if (sampler && sampler->matches(config))
        {
            return sampler;
        }
    }

    // No matching sampler found
    return nullptr;
}

auto SamplerPool::getCreateInfoFromType(PresetSamplerType type) -> SamplerCreateInfo
{
    SamplerCreateInfo createInfo;

    // Configure based on sampler type
    switch (type)
    {
    // Linear filtering samplers with mipmaps
    case PresetSamplerType::eLinearClampMipmap:
        createInfo.minFilter   = Filter::Linear;
        createInfo.magFilter   = Filter::Linear;
        createInfo.mipMapMode  = SamplerMipmapMode::Linear;
        createInfo.addressU    = SamplerAddressMode::ClampToEdge;
        createInfo.addressV    = SamplerAddressMode::ClampToEdge;
        createInfo.addressW    = SamplerAddressMode::ClampToEdge;
        createInfo.setLodRange = true;
        createInfo.minLod      = 0.0f;
        createInfo.maxLod      = 16.0f;
        break;

    case PresetSamplerType::eLinearWrapMipmap:
        createInfo.minFilter   = Filter::Linear;
        createInfo.magFilter   = Filter::Linear;
        createInfo.mipMapMode  = SamplerMipmapMode::Linear;
        createInfo.addressU    = SamplerAddressMode::Repeat;
        createInfo.addressV    = SamplerAddressMode::Repeat;
        createInfo.addressW    = SamplerAddressMode::Repeat;
        createInfo.setLodRange = true;
        createInfo.minLod      = 0.0f;
        createInfo.maxLod      = 16.0f;
        break;

    case PresetSamplerType::eLinearMirrorMipmap:
        createInfo.minFilter   = Filter::Linear;
        createInfo.magFilter   = Filter::Linear;
        createInfo.mipMapMode  = SamplerMipmapMode::Linear;
        createInfo.addressU    = SamplerAddressMode::MirroredRepeat;
        createInfo.addressV    = SamplerAddressMode::MirroredRepeat;
        createInfo.addressW    = SamplerAddressMode::MirroredRepeat;
        createInfo.setLodRange = true;
        createInfo.minLod      = 0.0f;
        createInfo.maxLod      = 16.0f;
        break;

    // Nearest filtering samplers with mipmaps
    case PresetSamplerType::eNearestClampMipmap:
        createInfo.minFilter   = Filter::Nearest;
        createInfo.magFilter   = Filter::Nearest;
        createInfo.mipMapMode  = SamplerMipmapMode::Nearest;
        createInfo.addressU    = SamplerAddressMode::ClampToEdge;
        createInfo.addressV    = SamplerAddressMode::ClampToEdge;
        createInfo.addressW    = SamplerAddressMode::ClampToEdge;
        createInfo.setLodRange = true;
        createInfo.minLod      = 0.0f;
        createInfo.maxLod      = 16.0f;
        break;

    case PresetSamplerType::eNearestWrapMipmap:
        createInfo.minFilter   = Filter::Nearest;
        createInfo.magFilter   = Filter::Nearest;
        createInfo.mipMapMode  = SamplerMipmapMode::Nearest;
        createInfo.addressU    = SamplerAddressMode::Repeat;
        createInfo.addressV    = SamplerAddressMode::Repeat;
        createInfo.addressW    = SamplerAddressMode::Repeat;
        createInfo.setLodRange = true;
        createInfo.minLod      = 0.0f;
        createInfo.maxLod      = 16.0f;
        break;

    // Anisotropic filtering samplers
    case PresetSamplerType::eAnisotropicClamp:
        createInfo.minFilter     = Filter::Linear;
        createInfo.magFilter     = Filter::Linear;
        createInfo.mipMapMode    = SamplerMipmapMode::Linear;
        createInfo.addressU      = SamplerAddressMode::ClampToEdge;
        createInfo.addressV      = SamplerAddressMode::ClampToEdge;
        createInfo.addressW      = SamplerAddressMode::ClampToEdge;
        createInfo.maxAnisotropy = 16.0f; // Maximum quality anisotropic filtering
        createInfo.setLodRange   = true;
        createInfo.minLod        = 0.0f;
        createInfo.maxLod        = 16.0f;
        break;

    case PresetSamplerType::eAnisotropicWrap:
        createInfo.minFilter     = Filter::Linear;
        createInfo.magFilter     = Filter::Linear;
        createInfo.mipMapMode    = SamplerMipmapMode::Linear;
        createInfo.addressU      = SamplerAddressMode::Repeat;
        createInfo.addressV      = SamplerAddressMode::Repeat;
        createInfo.addressW      = SamplerAddressMode::Repeat;
        createInfo.maxAnisotropy = 16.0f; // Maximum quality anisotropic filtering
        createInfo.setLodRange   = true;
        createInfo.minLod        = 0.0f;
        createInfo.maxLod        = 16.0f;
        break;

    // Special purpose samplers
    case PresetSamplerType::eShadowPcf:
        // PCF shadow map sampler
        createInfo.minFilter   = Filter::Linear;
        createInfo.magFilter   = Filter::Linear;
        createInfo.mipMapMode  = SamplerMipmapMode::Nearest;
        createInfo.addressU    = SamplerAddressMode::ClampToEdge;
        createInfo.addressV    = SamplerAddressMode::ClampToEdge;
        createInfo.addressW    = SamplerAddressMode::ClampToEdge;
        createInfo.compareFunc = CompareOp::LessEqual;
        createInfo.setLodRange = true;
        createInfo.minLod      = 0.0f;
        createInfo.maxLod      = 0.0f; // No mipmaps for shadow maps
        break;

    case PresetSamplerType::eShadowEsm:
        // Exponential shadow map sampler
        createInfo.minFilter   = Filter::Linear;
        createInfo.magFilter   = Filter::Linear;
        createInfo.mipMapMode  = SamplerMipmapMode::Nearest;
        createInfo.addressU    = SamplerAddressMode::ClampToEdge;
        createInfo.addressV    = SamplerAddressMode::ClampToEdge;
        createInfo.addressW    = SamplerAddressMode::ClampToEdge;
        createInfo.setLodRange = true;
        createInfo.minLod      = 0.0f;
        createInfo.maxLod      = 0.0f;
        break;

    case PresetSamplerType::eCubemap:
        // Standard cubemap sampler
        createInfo.minFilter   = Filter::Linear;
        createInfo.magFilter   = Filter::Linear;
        createInfo.mipMapMode  = SamplerMipmapMode::Linear;
        createInfo.addressU    = SamplerAddressMode::ClampToEdge;
        createInfo.addressV    = SamplerAddressMode::ClampToEdge;
        createInfo.addressW    = SamplerAddressMode::ClampToEdge;
        createInfo.setLodRange = true;
        createInfo.minLod      = 0.0f;
        createInfo.maxLod      = 16.0f;
        break;

    case PresetSamplerType::eCubemapLow:
        // Low-quality cubemap sampler for performance-critical scenarios
        createInfo.minFilter   = Filter::Linear;
        createInfo.magFilter   = Filter::Linear;
        createInfo.mipMapMode  = SamplerMipmapMode::Nearest;
        createInfo.addressU    = SamplerAddressMode::ClampToEdge;
        createInfo.addressV    = SamplerAddressMode::ClampToEdge;
        createInfo.addressW    = SamplerAddressMode::ClampToEdge;
        createInfo.setLodRange = true;
        createInfo.minLod      = 0.0f;
        createInfo.maxLod      = 4.0f; // Limit the number of mip levels
        break;

    case PresetSamplerType::ePointClamp:
        // Point sampling with clamp (for pixel-perfect rendering)
        createInfo.minFilter   = Filter::Nearest;
        createInfo.magFilter   = Filter::Nearest;
        createInfo.mipMapMode  = SamplerMipmapMode::Nearest;
        createInfo.addressU    = SamplerAddressMode::ClampToEdge;
        createInfo.addressV    = SamplerAddressMode::ClampToEdge;
        createInfo.addressW    = SamplerAddressMode::ClampToEdge;
        createInfo.setLodRange = true;
        createInfo.minLod      = 0.0f;
        createInfo.maxLod      = 0.0f; // Only use the base level
        break;

    default:
        APH_ASSERT(false, "Invalid sampler type");
        break;
    }

    return createInfo;
}

} // namespace aph::vk
