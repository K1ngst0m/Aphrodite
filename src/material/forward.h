#pragma once

#include "common/enum.h"

namespace aph
{
// Forward declarations for all Material system classes/structs
class Material;
class MaterialTemplate;
class MaterialRegistry;
class ParameterLayout;

// Enums needed for function signatures
enum class MaterialDomain : uint8_t;
enum class DataType : uint8_t;
enum class MaterialFeatureBits : uint32_t;
using MaterialFeatureFlags = Flags<MaterialFeatureBits>;

// Forward declare TypeUtils namespace
namespace TypeUtils
{
[[nodiscard]] auto getTypeSize(DataType type) -> uint32_t;
[[nodiscard]] auto isTextureType(DataType type) -> bool;
[[nodiscard]] auto isVectorType(DataType type) -> bool;
[[nodiscard]] auto isMatrixType(DataType type) -> bool;
[[nodiscard]] auto isScalarType(DataType type) -> bool;
[[nodiscard]] auto getTypeAlignment(DataType type) -> uint32_t;
[[nodiscard]] auto getTypeName(DataType type) -> const char*;
} // namespace TypeUtils

} // namespace aph 