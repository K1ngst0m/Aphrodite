#pragma once

#include "allocator/objectPool.h"
#include "common/result.h"
#include "material/materialRegistry.h"
#include "materialAsset.h"
#include <toml++/toml.h>

namespace aph
{
/**
 * @brief Material loader for the resource system
 * 
 * This class handles loading material assets from disk and manages 
 * material resources through the resource loader system.
 */
class MaterialLoader
{
public:
    /**
     * @brief Construct a new MaterialLoader with a registry
     * 
     * @param pRegistry The material registry to use
     */
    explicit MaterialLoader(MaterialRegistry* pRegistry);

    /**
     * @brief Destroy the MaterialLoader and free any resources
     */
    ~MaterialLoader();

    /**
     * @brief Load a material asset from a file
     * 
     * @param loadInfo The material load information
     * @param ppOutAsset Pointer to receive the loaded material asset
     * @return Result Result of the operation
     */
    auto load(const MaterialLoadInfo& loadInfo, MaterialAsset** ppOutAsset) -> Result;

    /**
     * @brief Free a material asset and its resources
     * @param pAsset The material asset to free
     */
    void unload(MaterialAsset* pAsset);

    /**
     * @brief Save a material asset to a file
     * 
     * @param pAsset The material asset to save
     * @param path The file path to save to
     * @return Result Result of the operation
     */
    auto save(MaterialAsset* pAsset, const char* path) -> Result;

    /**
     * @brief Check if a material asset needs reloading
     * 
     * @param pAsset The material asset to check
     * @return true If the asset needs reloading
     * @return false If the asset doesn't need reloading
     */
    auto needsReload(MaterialAsset* pAsset) -> bool;

    /**
     * @brief Reload a material asset from disk
     * 
     * @param pAsset The material asset to reload
     * @return Result Result of the operation
     */
    auto reload(MaterialAsset* pAsset) -> Result;

    /**
     * @brief Update material assets that need hot reloading
     */
    void update();
private:
    /**
     * @brief Parse material parameters from TOML data
     * 
     * @param data The TOML data
     * @param pMaterial The material to populate
     * @return Result Result of the operation
     */
    auto parseFromTOML(const toml::table& data, Material* pMaterial) -> Result;

    /**
     * @brief Serialize a material asset to TOML format
     * 
     * @param pAsset The material asset to serialize
     * @param outData Reference to receive the serialized data
     * @return Result Result of the operation
     */
    auto serializeToTOML(MaterialAsset* pAsset, toml::table& outData) -> Result;

    /**
     * @brief Check if a file exists and get its timestamp
     * 
     * @param path The file path to check
     * @param outTimestamp Pointer to receive the timestamp
     * @return true If the file exists
     * @return false If the file doesn't exist
     */
    auto checkFileExists(std::string_view path, uint64_t* outTimestamp) -> bool;

    MaterialRegistry* m_pRegistry{ nullptr }; ///< Material registry
    HashMap<MaterialAsset*, uint64_t> m_hotReloadMaterials; ///< Materials that need hot reloading
    uint64_t m_lastHotReloadCheckTime; ///< Last time hot reload was checked
    ObjectPool<MaterialAsset> m_assetPool; ///< Object pool for material assets
};

} // namespace aph
