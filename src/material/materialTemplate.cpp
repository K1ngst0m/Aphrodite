#include "materialTemplate.h"
#include "common/logger.h"
#include "parameterLayout.h"

namespace aph
{

MaterialTemplate::MaterialTemplate(std::string_view name, MaterialDomain domain, MaterialFeatureFlags featureFlags)
    : m_name(name)
    , m_domain(domain)
    , m_featureFlags(featureFlags)
    , m_parameterLayout(new ParameterLayout())
{
}

MaterialTemplate::~MaterialTemplate()
{
    delete m_parameterLayout;
}

void MaterialTemplate::addParameter(const MaterialParameterDesc& parameter)
{
    APH_ASSERT(m_parameterLayout);
    
    // Check for duplicate parameter names
    for (const auto& existingParam : m_parameterLayout->getParameters())
    {
        if (existingParam.name == parameter.name)
        {
            APH_LOG_WARN("Parameter '{}' already exists in material template '{}'. Ignoring.", parameter.name, m_name);
            return;
        }
    }

    // Add parameter to layout
    m_parameterLayout->addParameter(parameter);
}

auto MaterialTemplate::getParameterLayout() const -> const ParameterLayout&
{
    APH_ASSERT(m_parameterLayout);
    return *m_parameterLayout;
}

void MaterialTemplate::setShaderCode(ShaderStage stage, std::string_view code)
{
    m_shaderCode[stage] = std::string(code);
}

auto MaterialTemplate::getShaderCode(ShaderStage stage) const -> std::string_view
{
    auto it = m_shaderCode.find(stage);
    if (it != m_shaderCode.end())
    {
        return it->second;
    }
    return "";
}

auto MaterialTemplate::getDomain() const -> MaterialDomain
{
    return m_domain;
}

auto MaterialTemplate::getFeatureFlags() const -> MaterialFeatureFlags
{
    return m_featureFlags;
}

auto MaterialTemplate::getName() const -> std::string_view
{
    return m_name;
}

} // namespace aph
