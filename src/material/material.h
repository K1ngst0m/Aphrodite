#pragma once

#include "common/hash.h"
#include "common/result.h"
#include "common/smallVector.h"
#include "forward.h"

namespace aph
{
/**
 * @brief Material instance that references a template and stores parameter values
 *
 * This class represents a concrete material instance that uses a template as its
 * definition and maintains a set of parameter values. It supports setting and
 * retrieving parameter values, as well as creating GPU resources for rendering.
 */
class Material
{
public:
    /**
     * @brief Create a new material instance based on a template
     * @param pTemplate The template that defines this material's parameters
     * @return A new material instance
     */
    explicit Material(const MaterialTemplate* pTemplate);
    ~Material();

    Material(const Material&)                    = delete;
    auto operator=(const Material&) -> Material& = delete;
    Material(Material&&)                         = default;
    auto operator=(Material&&) -> Material&      = default;

    /**
     * @brief Get the template that defines this material
     * @return Pointer to the material template
     */
    [[nodiscard]] auto getTemplate() const -> const MaterialTemplate*;

    /**
     * @brief Set a float parameter value
     * @param name Parameter name
     * @param value Parameter value
     * @return Result indicating success or error
     */
    auto setFloat(std::string_view name, float value) -> Result;

    /**
     * @brief Set a vec2 parameter value
     * @param name Parameter name
     * @param value Parameter value
     * @return Result indicating success or error
     */
    auto setVec2(std::string_view name, const float* value) -> Result;

    /**
     * @brief Set a vec3 parameter value
     * @param name Parameter name
     * @param value Parameter value
     * @return Result indicating success or error
     */
    auto setVec3(std::string_view name, const float* value) -> Result;

    /**
     * @brief Set a vec4 parameter value
     * @param name Parameter name
     * @param value Parameter value
     * @return Result indicating success or error
     */
    auto setVec4(std::string_view name, const float* value) -> Result;

    /**
     * @brief Set a matrix parameter value
     * @param name Parameter name
     * @param value Parameter value
     * @return Result indicating success or error
     */
    auto setMat4(std::string_view name, const float* value) -> Result;

    /**
     * @brief Set a texture parameter value
     * @param name Parameter name
     * @param texturePath Path to the texture resource
     * @return Result indicating success or error
     */
    auto setTexture(std::string_view name, std::string_view texturePath) -> Result;

    /**
     * @brief Get the raw parameter data buffer
     * @return Pointer to parameter data
     */
    [[nodiscard]] auto getParameterData() const -> const void*;

    /**
     * @brief Get the size of the parameter data in bytes
     * @return Size in bytes
     */
    [[nodiscard]] auto getParameterDataSize() const -> size_t;

    /**
     * @brief Get list of texture bindings
     * @return Vector of texture paths
     */
    [[nodiscard]] auto getTextureBindings() const -> const HashMap<std::string, std::string>&;

private:
    /**
     * @brief Mark the parameter data as dirty, requiring GPU update
     */
    void markDirty();

    /**
     * @brief Check if the parameter data is dirty
     * @return True if data needs to be uploaded to GPU
     */
    [[nodiscard]] auto isDirty() const -> bool;

private:
    struct ParameterOffsetInfo
    {
        std::string name;
        uint32_t offset;
        DataType type;
        uint32_t size;
    };

    /**
     * @brief Find a parameter by name and validate its type
     * @param name Parameter name
     * @param expectedType Expected data type
     * @return Info about the parameter if found, or nullptr
     */
    auto findParameter(std::string_view name, DataType expectedType) -> ParameterOffsetInfo*;

    /**
     * @brief Initialize parameter storage from template
     */
    void initializeParameterStorage();

private:
    const MaterialTemplate* m_pTemplate{ nullptr };
    SmallVector<uint8_t> m_parameterData;
    SmallVector<ParameterOffsetInfo> m_parameterOffsets;
    HashMap<std::string, std::string> m_textureBindings;
    bool m_isDirty{ true };
};

} // namespace aph
