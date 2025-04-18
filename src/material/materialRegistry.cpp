#include "materialRegistry.h"
#include "common/logger.h"
#include "material.h"
#include "materialTemplate.h"

namespace aph
{
MaterialRegistry::MaterialRegistry() = default;

MaterialRegistry::~MaterialRegistry()
{
    // Clear templates hashmap - templates will be freed automatically when m_templatePool is destroyed
    m_templates.clear();
}

// Static factory method to create material registry
auto MaterialRegistry::Create() -> Expected<MaterialRegistry*>
{
    APH_LOG_INFO("Creating MaterialRegistry");

    // Create the registry
    auto* pRegistry = new MaterialRegistry();
    if (!pRegistry)
    {
        return { Result::RuntimeError, "Failed to allocate MaterialRegistry instance" };
    }

    // Register built-in templates
    pRegistry->registerBuiltInTemplates();

    return pRegistry;
}

// Static destroy method to clean up the registry
void MaterialRegistry::Destroy(MaterialRegistry* pRegistry)
{
    if (pRegistry)
    {
        APH_LOG_INFO("Destroying MaterialRegistry");
        delete pRegistry;
    }
}

auto MaterialRegistry::createTemplate(std::string_view name, MaterialDomain domain, MaterialFeatureFlags featureFlags)
    -> Expected<MaterialTemplate*>
{
    // Create template using thread-safe object pool
    MaterialTemplate* pTemplate = m_templatePool.allocate(name, domain, featureFlags);
    if (!pTemplate)
    {
        APH_LOG_ERR("Failed to allocate material template '%s'", std::string(name).c_str());
        return { Result::RuntimeError, "Failed to allocate material template" };
    }

    // Register the template
    auto result = registerTemplate(pTemplate);
    if (!result)
    {
        // Free the template if registration failed
        m_templatePool.free(pTemplate);
        return result;
    }

    return pTemplate;
}

auto MaterialRegistry::registerTemplate(MaterialTemplate* pTemplate) -> Expected<MaterialTemplate*>
{
    if (!pTemplate)
    {
        APH_LOG_ERR("Attempted to register null material template");
        return { Result::RuntimeError, "Attempted to register null material template" };
    }

    std::string name{ pTemplate->getName() };

    if (m_templates.contains(name))
    {
        APH_LOG_WARN("Material template '%s' already exists in registry. Overwriting.", name.c_str());

        // Free the old template if it exists
        if (auto it = m_templates.find(name); it != m_templates.end())
        {
            // Note: We don't free externally provided templates, only those we created
            // m_templatePool.free(it->second);
        }
    }

    m_templates[name] = pTemplate;

    APH_LOG_INFO("Registered material template '%s'", name.c_str());
    return pTemplate;
}

auto MaterialRegistry::findTemplate(std::string_view name) const -> Expected<MaterialTemplate*>
{
    if (auto it = m_templates.find(std::string{ name }); it != m_templates.end())
    {
        return it->second;
    }

    std::string errorMsg = "Material template '";
    errorMsg += name;
    errorMsg += "' not found";
    return { Result::RuntimeError, errorMsg };
}

auto MaterialRegistry::createMaterial(std::string_view templateName) -> Expected<Material*>
{
    auto templateResult = findTemplate(templateName);
    if (!templateResult)
    {
        return { templateResult.error().code, templateResult.error().message };
    }

    return createMaterial(templateResult.value());
}

auto MaterialRegistry::createMaterial(const MaterialTemplate* pTemplate) -> Expected<Material*>
{
    APH_ASSERT(pTemplate, "Failed to create material: null template provided");

    APH_LOG_INFO("Creating material from template '%s'", std::string(pTemplate->getName()).c_str());

    Material* pMaterial = m_materialPool.allocate(pTemplate);
    if (!pMaterial)
    {
        return { Result::RuntimeError, "Failed to allocate Material instance" };
    }

    return pMaterial;
}

void MaterialRegistry::freeMaterial(Material* pMaterial)
{
    if (!pMaterial)
    {
        return;
    }

    // Get template name for logging before freeing
    std::string templateName;
    if (pMaterial->getTemplate())
    {
        templateName = std::string(pMaterial->getTemplate()->getName());
    }

    // Free the material through the object pool
    m_materialPool.free(pMaterial);

    if (!templateName.empty())
    {
        APH_LOG_INFO("Freed material from template '%s'", templateName.c_str());
    }
}

void MaterialRegistry::freeTemplate(MaterialTemplate* pTemplate)
{
    if (!pTemplate)
    {
        return;
    }

    // Get template name for logging
    std::string name = std::string(pTemplate->getName());

    // Remove from templates map
    m_templates.erase(name);

    // Free the template through the object pool
    m_templatePool.free(pTemplate);

    APH_LOG_INFO("Freed material template '%s'", name.c_str());
}

auto MaterialRegistry::getTemplates() const -> const HashMap<std::string, MaterialTemplate*>&
{
    return m_templates;
}

void MaterialRegistry::registerBuiltInTemplates()
{
    APH_LOG_INFO("Registering built-in material templates");

    // Create standard PBR template
    auto pStandardPBRResult =
        createTemplate("StandardPBR", MaterialDomain::eOpaque,
                       MaterialFeatureBits::eStandard | MaterialFeatureBits::eEmissive | MaterialFeatureBits::eAO);

    if (!pStandardPBRResult)
    {
        APH_LOG_ERR("Failed to create StandardPBR template");
        return;
    }

    MaterialTemplate* pStandardPBR = pStandardPBRResult.value();

    // Add standard parameters
    pStandardPBR->addParameter(
        { .name = "baseColor", .type = DataType::eVec4, .offset = 0, .size = 16, .isTexture = false });

    pStandardPBR->addParameter(
        { .name = "metallic", .type = DataType::eFloat, .offset = 16, .size = 4, .isTexture = false });

    pStandardPBR->addParameter(
        { .name = "roughness", .type = DataType::eFloat, .offset = 20, .size = 4, .isTexture = false });

    // Add texture parameters
    pStandardPBR->addParameter(
        { .name = "albedoMap", .type = DataType::eTexture2D, .offset = 0, .size = 8, .isTexture = true });

    pStandardPBR->addParameter(
        { .name = "normalMap", .type = DataType::eTexture2D, .offset = 8, .size = 8, .isTexture = true });

    pStandardPBR->addParameter(
        { .name = "metallicRoughnessMap", .type = DataType::eTexture2D, .offset = 16, .size = 8, .isTexture = true });
}

} // namespace aph
