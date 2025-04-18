#include "material.h"
#include "common/logger.h"
#include "materialTemplate.h"
#include "parameterLayout.h"

namespace aph
{

Material::Material(const MaterialTemplate* pTemplate)
    : m_pTemplate(pTemplate)
{
    APH_ASSERT(pTemplate);
    initializeParameterStorage();
}

Material::~Material() = default;

auto Material::getTemplate() const -> const MaterialTemplate*
{
    return m_pTemplate;
}

void Material::initializeParameterStorage()
{
    APH_ASSERT(m_pTemplate);

    // Get the parameter layout from the template
    const auto& paramLayout = m_pTemplate->getParameterLayout();
    const auto& alignedParams = paramLayout.getAlignedLayout();
    
    // Get total size for uniform buffer
    uint32_t totalSize = paramLayout.getTotalSize();

    // Allocate parameter storage
    m_parameterData.resize(totalSize);

    // Initialize to zero
    std::memset(m_parameterData.data(), 0, totalSize);

    // Store parameter offset information for quick lookup
    for (const auto& param : alignedParams)
    {
        if (!param.isTexture)
        {
            m_parameterOffsets.push_back(
                { .name = std::string{ param.name }, .offset = param.offset, .type = param.type, .size = param.size });
        }
        else
        {
            // Just register texture parameters with empty path
            m_textureBindings[std::string{ param.name }] = "";
        }
    }

    APH_LOG_INFO("Initialized material using template '{}' with {} parameters and {} textures", m_pTemplate->getName(),
                 m_parameterOffsets.size(), m_textureBindings.size());
}

auto Material::findParameter(std::string_view name, DataType expectedType) -> Material::ParameterOffsetInfo*
{
    for (auto& param : m_parameterOffsets)
    {
        if (param.name == name)
        {
            if (param.type != expectedType)
            {
                APH_LOG_ERR("Parameter type mismatch for '{}': expected {}, got {}", name,
                            static_cast<int>(expectedType), static_cast<int>(param.type));
                return nullptr;
            }
            return &param;
        }
    }

    APH_LOG_ERR("Parameter '{}' not found in material", name);
    return nullptr;
}

auto Material::setFloat(std::string_view name, float value) -> Result
{
    auto* param = findParameter(name, DataType::eFloat);
    if (!param)
    {
        return Result::RuntimeError;
    }

    auto* dst = reinterpret_cast<float*>(m_parameterData.data() + param->offset);
    *dst      = value;
    markDirty();

    return Result::Success;
}

auto Material::setVec2(std::string_view name, const float* value) -> Result
{
    auto* param = findParameter(name, DataType::eVec2);
    if (!param)
    {
        return Result::RuntimeError;
    }

    float* dst = reinterpret_cast<float*>(m_parameterData.data() + param->offset);
    std::memcpy(dst, value, sizeof(float) * 2);
    markDirty();

    return Result::Success;
}

auto Material::setVec3(std::string_view name, const float* value) -> Result
{
    auto* param = findParameter(name, DataType::eVec3);
    if (!param)
    {
        return Result::RuntimeError;
    }

    auto* dst = reinterpret_cast<float*>(m_parameterData.data() + param->offset);
    std::memcpy(dst, value, sizeof(float) * 3);
    markDirty();

    return Result::Success;
}

auto Material::setVec4(std::string_view name, const float* value) -> Result
{
    auto* param = findParameter(name, DataType::eVec4);
    if (!param)
    {
        return Result::RuntimeError;
    }

    auto* dst = reinterpret_cast<float*>(m_parameterData.data() + param->offset);
    std::memcpy(dst, value, sizeof(float) * 4);
    markDirty();

    return Result::Success;
}

auto Material::setMat4(std::string_view name, const float* value) -> Result
{
    auto* param = findParameter(name, DataType::eMat4);
    if (!param)
    {
        return Result::RuntimeError;
    }

    auto* dst = reinterpret_cast<float*>(m_parameterData.data() + param->offset);
    std::memcpy(dst, value, sizeof(float) * 16);
    markDirty();

    return Result::Success;
}

auto Material::setTexture(std::string_view name, std::string_view texturePath) -> Result
{
    auto it = m_textureBindings.find(std::string{ name });
    if (it == m_textureBindings.end())
    {
        APH_LOG_ERR("Texture parameter '{}' not found in material", name);
        return Result::RuntimeError;
    }

    it->second = std::string{ texturePath };
    markDirty();

    return Result::Success;
}

auto Material::getParameterData() const -> const void*
{
    return m_parameterData.data();
}

auto Material::getParameterDataSize() const -> size_t
{
    return m_parameterData.size();
}

void Material::markDirty()
{
    m_isDirty = true;
}

auto Material::isDirty() const -> bool
{
    return m_isDirty;
}

auto Material::getTextureBindings() const -> const HashMap<std::string, std::string>&
{
    return m_textureBindings;
}

} // namespace aph
