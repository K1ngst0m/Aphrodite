#include "materialTemplate.h"
#include "common/logger.h"

namespace aph
{

MaterialTemplate::MaterialTemplate(std::string_view name, MaterialDomain domain, MaterialFeatureFlags featureFlags)
    : m_name(name)
    , m_domain(domain)
    , m_featureFlags(featureFlags)
{
}

void MaterialTemplate::addParameter(MaterialParameterDesc parameter)
{
    auto it = std::ranges::find_if(m_parameters,
                                   [&parameter](const MaterialParameterDesc& p)
                                   {
                                       return p.name == parameter.name;
                                   });

    if (it != m_parameters.end())
    {
        APH_LOG_WARN("Parameter '{}' already exists in material template '{}'. Ignoring.", parameter.name, m_name);
        return;
    }

    m_parameters.push_back(parameter);
}

void MaterialTemplate::setShaderCode(ShaderStage stage, std::string_view code)
{
    m_shaderCode[stage] = std::string(code);
}

auto MaterialTemplate::getParameterLayout() const -> const SmallVector<MaterialParameterDesc>&
{
    return m_parameters;
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
