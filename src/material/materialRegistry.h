#pragma once

#include "allocator/objectPool.h"
#include "common/result.h"
#include "material.h"
#include "materialTemplate.h"

namespace aph
{
// Forward declarations
class Material;

/**
 * @brief Registry for material templates and factory for material instances
 * 
 * This class manages a collection of material templates and provides factory methods
 * to create material instances based on these templates.
 * 
 * TODO @material: Implement the Material class and integrate with object pool
 * When Material class is fully implemented:
 * 1. Include "allocator/objectPool.h"
 * 2. Add ThreadSafeObjectPool<Material> m_materialPool as a private member
 * 3. Use m_materialPool.allocate() to create materials in createMaterial methods
 * 4. Materials will be automatically freed when m_materialPool is destroyed
 * 5. Provide a separate method to manually free individual materials if needed:
 *    void freeMaterial(Material* material) { if (material) m_materialPool.free(material); }
 */
class MaterialRegistry
{
public:
    // Lifecycle management
    static auto Create() -> Expected<MaterialRegistry*>;
    static void Destroy(MaterialRegistry* pRegistry);
    MaterialRegistry();
    ~MaterialRegistry();

    // Template management
    [[nodiscard]] auto createTemplate(std::string_view name, MaterialDomain domain, MaterialFeatureFlags featureFlags)
        -> Expected<MaterialTemplate*>;
    [[nodiscard]] auto registerTemplate(MaterialTemplate* pTemplate) -> Expected<MaterialTemplate*>;
    [[nodiscard]] auto findTemplate(std::string_view name) const -> Expected<MaterialTemplate*>;
    void freeTemplate(MaterialTemplate* pTemplate);
    [[nodiscard]] auto getTemplates() const -> const HashMap<std::string, MaterialTemplate*>&;

    // Material management
    [[nodiscard]] auto createMaterial(std::string_view templateName) -> Expected<Material*>;
    [[nodiscard]] auto createMaterial(const MaterialTemplate* pTemplate) -> Expected<Material*>;
    void freeMaterial(Material* pMaterial);

private:
    void registerBuiltInTemplates();

private:
    HashMap<std::string, MaterialTemplate*> m_templates;
    ThreadSafeObjectPool<MaterialTemplate> m_templatePool;
    ThreadSafeObjectPool<Material> m_materialPool;
};

} // namespace aph