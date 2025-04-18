#include "material.h"
#include "common/logger.h"
#include "parameterLayout.h"

namespace aph
{

Material::Material(MaterialTemplate* pTemplate)
    : m_pTemplate(pTemplate)
{
    APH_ASSERT(pTemplate);
    initializeParameterStorage();
}

Material::~Material() = default;

Material::Material(Material&& other) noexcept
    : m_pTemplate(other.m_pTemplate)
    , m_parameterData(std::move(other.m_parameterData))
    , m_parameterOffsets(std::move(other.m_parameterOffsets))
    , m_textureBindings(std::move(other.m_textureBindings))
    , m_isDirty(other.m_isDirty)
{
    other.m_pTemplate = nullptr;
    other.m_isDirty   = false;
}

Material& Material::operator=(Material&& other) noexcept
{
    if (this != &other)
    {
        m_pTemplate        = other.m_pTemplate;
        m_parameterData    = std::move(other.m_parameterData);
        m_parameterOffsets = std::move(other.m_parameterOffsets);
        m_textureBindings  = std::move(other.m_textureBindings);
        m_isDirty          = other.m_isDirty;

        other.m_pTemplate = nullptr;
        other.m_isDirty   = false;
    }
    return *this;
}

auto Material::getTemplate() const -> MaterialTemplate*
{
    return m_pTemplate;
}

void Material::initializeParameterStorage()
{
    APH_ASSERT(m_pTemplate);

    // Generate layout from template
    auto layout = ParameterLayout::generateLayout(m_pTemplate);

    // Calculate total size with proper alignment
    uint32_t totalSize = ParameterLayout::calculateTotalSize(layout);

    // Allocate parameter storage
    m_parameterData.resize(totalSize);

    // Initialize to zero
    std::memset(m_parameterData.data(), 0, totalSize);

    // Store parameter offset information for quick lookup
    for (const auto& param : layout)
    {
        if (!param.isTexture)
        {
            m_parameterOffsets.push_back({ std::string{ param.name }, param.offset, param.type, param.size });
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

    float* dst = reinterpret_cast<float*>(m_parameterData.data() + param->offset);
    *dst       = value;
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

    float* dst = reinterpret_cast<float*>(m_parameterData.data() + param->offset);
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

    float* dst = reinterpret_cast<float*>(m_parameterData.data() + param->offset);
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

    float* dst = reinterpret_cast<float*>(m_parameterData.data() + param->offset);
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

auto Material::updateGPUResources() -> Result
{
    // TODO: Implement GPU resource updates when GPU resource system is available
    // This would create/update uniform buffers and bind textures

    // Mark clean after update
    m_isDirty = false;

    return Result::Success;
}

auto Material::getTextureBindings() const -> const HashMap<std::string, std::string>&
{
    return m_textureBindings;
}

} // namespace aph