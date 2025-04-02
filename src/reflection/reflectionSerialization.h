#pragma once

#include "shaderReflector.h"
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>
#include <toml++/toml.h>

namespace aph
{
/**
 * Serializes reflection data to TOML format
 */
namespace reflection
{
/**
     * Serializes a bitset to a TOML array
     */
template <size_t N>
toml::array serializeBitset(const std::bitset<N>& bitset)
{
    toml::array result;

    // Store indices of all set bits
    for (size_t i = 0; i < N; i++)
    {
        if (bitset.test(i))
        {
            result.push_back(static_cast<int64_t>(i));
        }
    }

    return result;
}

/**
     * Deserializes a TOML array to a bitset
     */
template <size_t N>
std::bitset<N> deserializeBitset(const toml::array* arr)
{
    std::bitset<N> result;
    if (!arr)
        return result;

    // Set bits at indices specified in the array
    for (const auto& val : *arr)
    {
        if (val.is_integer())
        {
            auto index = val.as_integer()->get();
            if (index >= 0 && index < static_cast<int64_t>(N))
            {
                result.set(static_cast<size_t>(index));
            }
        }
    }

    return result;
}

/**
     * Serializes a ShaderLayout to a TOML table
     */
toml::table serializeShaderLayout(const ShaderLayout& layout);

/**
     * Deserializes a TOML table to a ShaderLayout
     */
ShaderLayout deserializeShaderLayout(const toml::table* table);

/**
     * Serializes a ResourceLayout to a TOML table
     */
toml::table serializeResourceLayout(const ResourceLayout& layout);

/**
     * Deserializes a TOML table to a ResourceLayout
     */
ResourceLayout deserializeResourceLayout(const toml::table* table);

/**
     * Serializes a CombinedResourceLayout to a TOML table
     */
toml::table serializeCombinedResourceLayout(const CombinedResourceLayout& layout);

/**
     * Deserializes a TOML table to a CombinedResourceLayout
     */
CombinedResourceLayout deserializeCombinedResourceLayout(const toml::table* table);

/**
     * Serializes a ReflectionResult to a TOML table
     */
toml::table serializeReflectionResult(const ReflectionResult& result);

/**
     * Deserializes a TOML table to a ReflectionResult
     */
ReflectionResult deserializeReflectionResult(const toml::table* table);

/**
     * Saves a ReflectionResult to a TOML file
     */
bool saveReflectionToFile(const ReflectionResult& result, const std::filesystem::path& path);

/**
     * Loads a ReflectionResult from a TOML file
     */
Result loadReflectionFromFile(const std::filesystem::path& path, ReflectionResult& result);
} // namespace reflection
} // namespace aph