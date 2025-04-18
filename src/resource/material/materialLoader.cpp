#include "materialLoader.h"
#include "common/logger.h"
#include "filesystem/filesystem.h"
#include "global/globalManager.h"
#include "materialAsset.h"
#include "material/materialTemplate.h"
#include <chrono>
#include <sstream>
#include <toml++/toml.h>

namespace aph
{
MaterialLoader::MaterialLoader(MaterialRegistry* pRegistry)
    : m_pRegistry(pRegistry)
    , m_lastHotReloadCheckTime(0)
{
    if (!m_pRegistry)
    {
        APH_LOG_ERR("MaterialLoader created without valid registry");
    }
    else
    {
        APH_LOG_INFO("MaterialLoader initialized");
    }
}

MaterialLoader::~MaterialLoader()
{
    // Clear hot reload materials (don't free them, they'll be freed by the pool)
    m_hotReloadMaterials.clear();

    // Release registry reference
    m_pRegistry = nullptr;

    APH_LOG_INFO("MaterialLoader destroyed");
}

auto MaterialLoader::load(const MaterialLoadInfo& loadInfo, MaterialAsset** ppOutAsset) -> Result
{
    if (!m_pRegistry)
    {
        APH_LOG_ERR("MaterialLoader has no registry");
        return Result::RuntimeError;
    }

    if (!ppOutAsset)
    {
        APH_LOG_ERR("Invalid output pointer");
        return Result::RuntimeError;
    }

    // Check if file exists
    uint64_t timestamp = 0;
    if (!checkFileExists(loadInfo.path, &timestamp))
    {
        APH_LOG_ERR("Failed to load material asset: file '%s' not found", loadInfo.path);
        return { Result::RuntimeError, "File not found" };
    }

    // Create a new material asset using object pool
    MaterialAsset* pAsset = m_assetPool.allocate();
    if (!pAsset)
    {
        APH_LOG_ERR("Failed to allocate MaterialAsset");
        return Result::RuntimeError;
    }

    // Set timestamp
    pAsset->m_timestamp = timestamp;

    // Read file content
    auto contentResult = APH_DEFAULT_FILESYSTEM.readFileToString(loadInfo.path);
    if (!contentResult.success())
    {
        APH_LOG_ERR("Failed to read material file: %s", loadInfo.path);
        m_assetPool.free(pAsset);
        return contentResult.error().code;
    }

    // Parse the TOML data
    try
    {
        // Parse TOML data
        toml::table tomlTable = toml::parse(std::string(contentResult.value()));

        // Create material from the TOML data
        auto result = parseFromTOML(tomlTable, pAsset->m_pMaterial);
        if (result.success())
        {
            pAsset->m_path       = std::string(loadInfo.path);
            pAsset->m_isModified = false;

            // Add to hot reload tracking if enabled
            if (loadInfo.enableHotReload)
            {
                m_hotReloadMaterials[pAsset] = timestamp;
            }

            // Store the output
            *ppOutAsset = pAsset;

            // Log success with debug name
            if (!loadInfo.debugName.empty())
            {
                APH_LOG_INFO("Successfully loaded material '%s'", loadInfo.debugName);
            }
            else
            {
                APH_LOG_INFO("Successfully loaded material from '%s'", loadInfo.path);
            }
        }
        else
        {
            m_assetPool.free(pAsset);
        }

        return result;
    }
    catch (const toml::parse_error& err)
    {
        APH_LOG_ERR("Failed to parse material TOML: %s", err.what());
        m_assetPool.free(pAsset);
        return { Result::RuntimeError, "TOML parse error" };
    }
}

void MaterialLoader::unload(MaterialAsset* pAsset)
{
    if (!pAsset)
    {
        return;
    }

    // Free the owned material if necessary
    if (pAsset->getMaterial() && m_pRegistry)
    {
        m_pRegistry->freeMaterial(pAsset->getMaterial());
        pAsset->m_pMaterial = nullptr;
    }

    // Remove from hot reload tracking
    m_hotReloadMaterials.erase(pAsset);
}

auto MaterialLoader::save(MaterialAsset* pAsset, const char* path) -> Result
{
    if (!pAsset || !pAsset->m_pMaterial)
    {
        APH_LOG_ERR("Cannot save material asset: no material loaded");
        return Result::RuntimeError;
    }

    // Serialize to TOML
    toml::table tomlData;
    auto result = serializeToTOML(pAsset, tomlData);
    if (!result.success())
    {
        return result;
    }

    // Convert to string
    std::stringstream ss;
    ss << tomlData;
    std::string tomlString = ss.str();

    // Write file using filesystem API
    auto writeResult = APH_DEFAULT_FILESYSTEM.writeStringToFile(path, tomlString);
    if (!writeResult.success())
    {
        APH_LOG_ERR("Failed to write material file: %s", path);
        return writeResult;
    }

    // Update path and timestamp in asset
    uint64_t timestamp = 0;
    checkFileExists(path, &timestamp);

    pAsset->m_path       = path;
    pAsset->m_timestamp  = timestamp;
    pAsset->m_isModified = false;

    APH_LOG_INFO("Successfully saved material to '%s'", path);

    return Result::Success;
}

auto MaterialLoader::needsReload(MaterialAsset* pAsset) -> bool
{
    if (!pAsset || pAsset->m_path.empty())
    {
        // Not loaded from disk
        return false;
    }

    uint64_t currentTimestamp = 0;
    if (!checkFileExists(pAsset->m_path, &currentTimestamp))
    {
        // File no longer exists
        return false;
    }

    // Check if file has been modified since we loaded it
    return currentTimestamp > pAsset->m_timestamp;
}

auto MaterialLoader::reload(MaterialAsset* pAsset) -> Result
{
    if (!pAsset || pAsset->m_path.empty())
    {
        APH_LOG_ERR("Cannot reload material asset: not loaded from disk");
        return Result::RuntimeError;
    }

    if (!needsReload(pAsset))
    {
        // No need to reload
        return Result::Success;
    }

    // Store a copy of the path
    std::string path = pAsset->m_path;

    // If we own the material, we need to backup a reference
    Material* oldMaterial = pAsset->m_pMaterial;
    pAsset->m_pMaterial   = nullptr;

    // Read file content
    auto contentResult = APH_DEFAULT_FILESYSTEM.readFileToString(path);
    if (!contentResult.success())
    {
        APH_LOG_ERR("Failed to read material file: %s", path);
        // Restore old material if reading failed
        pAsset->m_pMaterial = oldMaterial;
        return contentResult.error().code;
    }

    // Update timestamp
    uint64_t timestamp = 0;
    checkFileExists(path, &timestamp);
    pAsset->m_timestamp = timestamp;

    // Parse the TOML data
    try
    {
        // Parse TOML data
        toml::table tomlTable = toml::parse(std::string(contentResult.value()));

        // Create material from the TOML data
        auto result = parseFromTOML(tomlTable, pAsset->m_pMaterial);

        if (!result.success() && oldMaterial != nullptr)
        {
            // Restore old material if parsing failed
            pAsset->m_pMaterial = oldMaterial;
            APH_LOG_WARN("Failed to reload material from '%s', keeping old version", path);
        }
        else if (oldMaterial != nullptr && m_pRegistry)
        {
            // Free old material
            m_pRegistry->freeMaterial(oldMaterial);
        }

        if (result.success())
        {
            APH_LOG_INFO("Successfully reloaded material from '%s'", path);
        }

        return result;
    }
    catch (const toml::parse_error& err)
    {
        APH_LOG_ERR("Failed to parse material TOML: %s", err.what());
        if (oldMaterial != nullptr)
        {
            pAsset->m_pMaterial = oldMaterial;
        }
        return { Result::RuntimeError, "TOML parse error" };
    }
}

void MaterialLoader::update()
{
    // Calculate current time for rate limiting hot reload checks
    uint64_t currentTime =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();

    // Don't check too frequently (e.g., check every 500ms)
    if (currentTime - m_lastHotReloadCheckTime < 500)
    {
        return;
    }

    m_lastHotReloadCheckTime = currentTime;

    // Check all materials registered for hot reload
    for (auto it = m_hotReloadMaterials.begin(); it != m_hotReloadMaterials.end();)
    {
        MaterialAsset* pAsset = it->first;

        // Check if this material needs to be reloaded
        if (needsReload(pAsset))
        {
            // Reload the material and check for errors
            auto result = reload(pAsset);
            if (result.success())
            {
                // Update timestamp in the hot reload map
                it->second = pAsset->m_timestamp;
            }
            ++it;
        }
        else
        {
            ++it;
        }
    }
}

auto MaterialLoader::parseFromTOML(const toml::table& data, Material* pMaterial) -> Result
{
    try
    {
        // Extract template name
        const auto* materialSection = data.get_as<toml::table>("material");
        if (!materialSection)
        {
            APH_LOG_ERR("Failed to parse material: [material] section not found");
            return { Result::RuntimeError, "Format error: [material] section not found" };
        }

        const auto* templateNode = materialSection->get("template");
        if (!templateNode || !templateNode->is_string())
        {
            APH_LOG_ERR("Failed to parse material: template name not found or invalid");
            return { Result::RuntimeError, "Format error: template name not found" };
        }

        std::string templateName = templateNode->as_string()->get();
        if (templateName.empty())
        {
            APH_LOG_ERR("Failed to parse material: empty template name");
            return { Result::RuntimeError, "Format error: empty template name" };
        }

        // Create material from template
        auto materialResult = m_pRegistry->createMaterial(templateName);
        if (!materialResult)
        {
            APH_LOG_ERR("Failed to create material: template '%s' not found", templateName);
            return Result{ Result::RuntimeError, "Template not found" };
        }

        pMaterial = materialResult.value();
        if (!pMaterial)
        {
            APH_LOG_ERR("Failed to create material: null material returned");
            return Result{ Result::RuntimeError, "Failed to create material" };
        }

        // Set material properties from TOML
        const auto* propertiesSection = materialSection->get_as<toml::table>("properties");
        if (propertiesSection)
        {
            // Iterate through properties
            for (auto& [name, value] : *propertiesSection)
            {
                std::string propName = std::string(name.str());

                // Handle different parameter types based on TOML value types
                if (value.is_floating_point())
                {
                    auto floatValue = static_cast<float>(value.as_floating_point()->get());
                    auto result     = pMaterial->setFloat(propName, floatValue);
                    if (!result.success())
                    {
                        APH_LOG_WARN("Failed to set float parameter '%s'", propName);
                    }
                }
                else if (value.is_integer())
                {
                    auto floatValue = static_cast<float>(value.as_integer()->get());
                    auto result     = pMaterial->setFloat(propName, floatValue);
                    if (!result.success())
                    {
                        APH_LOG_WARN("Failed to set float parameter '%s'", propName);
                    }
                }
                else if (value.is_boolean())
                {
                    // Handle boolean values - convert to float (0.0 or 1.0)
                    float floatValue = value.as_boolean()->get() ? 1.0f : 0.0f;
                    auto result      = pMaterial->setFloat(propName, floatValue);
                    if (!result.success())
                    {
                        APH_LOG_WARN("Failed to set float parameter '%s'", propName);
                    }
                }
                else if (value.is_array())
                {
                    const auto* array = value.as_array();
                    size_t size       = array->size();

                    if (size == 2)
                    {
                        // Handle Vector2
                        float vec2[2] = { static_cast<float>((*array)[0].as_floating_point()->get()),
                                          static_cast<float>((*array)[1].as_floating_point()->get()) };
                        auto result   = pMaterial->setVec2(propName, vec2);
                        if (!result.success())
                        {
                            APH_LOG_WARN("Failed to set vec2 parameter '%s'", propName);
                        }
                    }
                    else if (size == 3)
                    {
                        // Handle Vector3
                        float vec3[3] = { static_cast<float>((*array)[0].as_floating_point()->get()),
                                          static_cast<float>((*array)[1].as_floating_point()->get()),
                                          static_cast<float>((*array)[2].as_floating_point()->get()) };
                        auto result   = pMaterial->setVec3(propName, vec3);
                        if (!result.success())
                        {
                            APH_LOG_WARN("Failed to set vec3 parameter '%s'", propName);
                        }
                    }
                    else if (size == 4)
                    {
                        // Handle Vector4/Color
                        float vec4[4] = { static_cast<float>((*array)[0].as_floating_point()->get()),
                                          static_cast<float>((*array)[1].as_floating_point()->get()),
                                          static_cast<float>((*array)[2].as_floating_point()->get()),
                                          static_cast<float>((*array)[3].as_floating_point()->get()) };
                        auto result   = pMaterial->setVec4(propName, vec4);
                        if (!result.success())
                        {
                            APH_LOG_WARN("Failed to set vec4 parameter '%s'", propName);
                        }
                    }
                    else
                    {
                        APH_LOG_WARN("Array parameter '%s' has unsupported size %zu", propName, size);
                    }
                }
                else if (value.is_string())
                {
                    // Assume texture parameters are strings
                    std::string texturePath = value.as_string()->get();
                    auto result             = pMaterial->setTexture(propName, texturePath);
                    if (!result.success())
                    {
                        APH_LOG_WARN("Failed to set texture parameter '%s'", propName);
                    }
                }
                else
                {
                    APH_LOG_WARN("Unsupported TOML value type for parameter '%s'", propName);
                }
            }
        }

        return Result::Success;
    }
    catch (const std::exception& e)
    {
        APH_LOG_ERR("Error parsing material TOML: %s", e.what());
        return Result{ Result::RuntimeError, "Parse error" };
    }
}

auto MaterialLoader::serializeToTOML(MaterialAsset* pAsset, toml::table& outData) -> Result
{
    if (!pAsset || !pAsset->m_pMaterial)
    {
        APH_LOG_ERR("Cannot serialize null material");
        return Result::RuntimeError;
    }

    try
    {
        // Create material section
        toml::table materialSection;

        // Add template name
        Material* material = pAsset->m_pMaterial;
        if (!material)
        {
            return { Result::RuntimeError, "Null material in asset" };
        }

        const MaterialTemplate* materialTemplate = material->getTemplate();
        if (!materialTemplate)
        {
            return { Result::RuntimeError, "Null template in material" };
        }

        materialSection.insert("template", std::string(materialTemplate->getName()));

        // Create properties section
        toml::table propertiesSection;

        // Add properties section to material section
        materialSection.insert("properties", propertiesSection);

        // Add material section to root
        outData.insert("material", materialSection);

        return Result::Success;
    }
    catch (const std::exception& e)
    {
        APH_LOG_ERR("Error serializing material TOML: %s", e.what());
        return Result{ Result::RuntimeError, "Serialization error" };
    }
}

auto MaterialLoader::checkFileExists(std::string_view path, uint64_t* outTimestamp) -> bool
{
    if (path.empty() || !outTimestamp)
    {
        return false;
    }

    if (!APH_DEFAULT_FILESYSTEM.exist(path))
    {
        return false;
    }

    *outTimestamp = APH_DEFAULT_FILESYSTEM.getLastModifiedTime(path);
    return true;
}

} // namespace aph
