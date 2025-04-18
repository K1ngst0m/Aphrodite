#pragma once

#include "material/material.h"
#include <string>

namespace aph
{
// Forward declarations
class MaterialRegistry;
class MaterialLoader;

/**
 * @brief Load information for material assets
 */
struct MaterialLoadInfo
{
    std::string debugName; //!< Debug name for the material
    std::string path; //!< Path to the material file

    // Optional parameters
    bool enableHotReload{ false }; //!< Enable automatic hot reload when file changes
};

/**
 * @brief Asset class for material resources
 *
 * This class holds loaded material data and provides access to it.
 */
class MaterialAsset
{
    friend class MaterialLoader; // Allow loader to modify internals directly

public:
    MaterialAsset() = default;
    ~MaterialAsset() = default;
    /**
     * @brief Create a material asset from an existing material
     * @param pMaterial Pointer to the material
     */
    explicit MaterialAsset(Material* pMaterial);

    MaterialAsset(const MaterialAsset&)                        = delete;
    auto operator=(const MaterialAsset&) -> MaterialAsset&     = delete;
    MaterialAsset(MaterialAsset&&) noexcept                    = default;
    auto operator=(MaterialAsset&&) noexcept -> MaterialAsset& = default;

    /**
     * @brief Check if the material has been loaded
     * @return True if the material is loaded
     */
    [[nodiscard]] auto isLoaded() const -> bool;

    /**
     * @brief Get the material
     * @return Pointer to the material or nullptr if not loaded
     */
    [[nodiscard]] auto getMaterial() const -> Material*;

    /**
     * @brief Get the material asset path if it was loaded from disk
     * @return Material asset path or empty string if not loaded from disk
     */
    [[nodiscard]] auto getPath() const -> std::string_view;

    /**
     * @brief Check if the material asset has been modified since loading
     * @return True if the asset has been modified
     */
    [[nodiscard]] auto isModified() const -> bool;

    /**
     * @brief Get the timestamp of the last file modification
     * @return Timestamp of the last file modification
     */
    [[nodiscard]] auto getTimestamp() const -> uint64_t;

private:
    Material* m_pMaterial{ nullptr };
    std::string m_path;
    uint64_t m_timestamp{ 0 };
    bool m_isModified{ false };
};

} // namespace aph
