#pragma once

#include "allocator/objectPool.h"
#include "common/result.h"
#include "forward.h"

namespace aph
{
/**
 * @brief Registry for material templates and factory for material instances
 * 
 * This class manages a collection of material templates and provides factory methods
 * to create material instances based on these templates.
 */
class MaterialRegistry
{
public:
    // Factory methods
    static auto Create() -> Expected<MaterialRegistry*>;
    static void Destroy(MaterialRegistry* pRegistry);

public:
    // Template management
    [[nodiscard]] auto createTemplate(std::string_view name, MaterialDomain domain, MaterialFeatureFlags featureFlags)
        -> Expected<MaterialTemplate*>;
    [[nodiscard]] auto registerTemplate(MaterialTemplate* pTemplate) -> Expected<MaterialTemplate*>;
    [[nodiscard]] auto findTemplate(std::string_view name) const -> Expected<MaterialTemplate*>;
    void freeTemplate(MaterialTemplate* pTemplate);
    [[nodiscard]] auto getTemplates() const -> const HashMap<std::string, MaterialTemplate*>&;

public:
    // Material management
    [[nodiscard]] auto createMaterial(std::string_view templateName) -> Expected<Material*>;
    [[nodiscard]] auto createMaterial(const MaterialTemplate* pTemplate) -> Expected<Material*>;
    void freeMaterial(Material* pMaterial);

private:
    MaterialRegistry();
    ~MaterialRegistry();
    void registerBuiltInTemplates();

private:
    HashMap<std::string, MaterialTemplate*> m_templates;
    ThreadSafeObjectPool<MaterialTemplate> m_templatePool;
    ThreadSafeObjectPool<Material> m_materialPool;
};

} // namespace aph
