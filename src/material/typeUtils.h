#pragma once

#include "materialTemplate.h"
#include "math/math.h"

namespace aph::TypeUtils
{
// Get the size of a data type
[[nodiscard]] inline auto getTypeSize(DataType type) -> uint32_t
{
    switch (type)
    {
    case DataType::eFloat:
    case DataType::eInt:
    case DataType::eUint:
    case DataType::eBool:
        return 4;
    case DataType::eVec2:
    case DataType::eIvec2:
    case DataType::eUvec2:
        return 8;
    case DataType::eVec3:
    case DataType::eIvec3:
    case DataType::eUvec3:
        return 12;
    case DataType::eVec4:
    case DataType::eIvec4:
    case DataType::eUvec4:
        return 16;
    case DataType::eMat2:
        return 16;
    case DataType::eMat3:
        return 36;
    case DataType::eMat4:
        return 64;
    case DataType::eTexture2D:
    case DataType::eTextureCube:
    case DataType::eTexture2DArray:
    case DataType::eTexture3D:
    case DataType::eSampler:
        return 8; // Handle/descriptor size (implementation dependent)
    case DataType::eBuffer:
        return 8; // Buffer handle size
    default:
        return 0;
    }
}

// Check if a type is a texture type
[[nodiscard]] inline auto isTextureType(DataType type) -> bool
{
    return type == DataType::eTexture2D || type == DataType::eTextureCube || type == DataType::eTexture2DArray ||
           type == DataType::eTexture3D;
}

// Check if a type is a vector type
[[nodiscard]] inline auto isVectorType(DataType type) -> bool
{
    return type == DataType::eVec2 || type == DataType::eVec3 || type == DataType::eVec4 || type == DataType::eIvec2 ||
           type == DataType::eIvec3 || type == DataType::eIvec4 || type == DataType::eUvec2 ||
           type == DataType::eUvec3 || type == DataType::eUvec4;
}

// Check if a type is a matrix type
[[nodiscard]] inline auto isMatrixType(DataType type) -> bool
{
    return type == DataType::eMat2 || type == DataType::eMat3 || type == DataType::eMat4;
}

// Check if a type is a scalar type
[[nodiscard]] inline auto isScalarType(DataType type) -> bool
{
    return type == DataType::eFloat || type == DataType::eInt || type == DataType::eUint || type == DataType::eBool;
}

// Get alignment requirement for a type
[[nodiscard]] inline auto getTypeAlignment(DataType type) -> uint32_t
{
    switch (type)
    {
    case DataType::eFloat:
    case DataType::eInt:
    case DataType::eUint:
    case DataType::eBool:
        return 4;
    case DataType::eVec2:
    case DataType::eIvec2:
    case DataType::eUvec2:
        return 8;
    case DataType::eVec3:
    case DataType::eIvec3:
    case DataType::eUvec3:
        return 16; // Vec3 typically aligned to 16 bytes in shaders
    case DataType::eVec4:
    case DataType::eIvec4:
    case DataType::eUvec4:
    case DataType::eMat2:
    case DataType::eMat3:
    case DataType::eMat4:
        return 16;
    case DataType::eTexture2D:
    case DataType::eTextureCube:
    case DataType::eTexture2DArray:
    case DataType::eTexture3D:
    case DataType::eSampler:
    case DataType::eBuffer:
        return 8;
    default:
        return 4;
    }
}

// Template to get the DataType enum value for a C++ type
template <typename T>
struct TypeToEnum
{
};

template <>
struct TypeToEnum<float>
{
    static constexpr DataType value = DataType::eFloat;
};

template <>
struct TypeToEnum<int32_t>
{
    static constexpr DataType value = DataType::eInt;
};

template <>
struct TypeToEnum<uint32_t>
{
    static constexpr DataType value = DataType::eUint;
};

template <>
struct TypeToEnum<bool>
{
    static constexpr DataType value = DataType::eBool;
};

template <>
struct TypeToEnum<Vec2>
{
    static constexpr DataType value = DataType::eVec2;
};

template <>
struct TypeToEnum<Vec3>
{
    static constexpr DataType value = DataType::eVec3;
};

template <>
struct TypeToEnum<Vec4>
{
    static constexpr DataType value = DataType::eVec4;
};

template <>
struct TypeToEnum<Vec2i>
{
    static constexpr DataType value = DataType::eIvec2;
};

template <>
struct TypeToEnum<Vec3i>
{
    static constexpr DataType value = DataType::eIvec3;
};

template <>
struct TypeToEnum<Vec4i>
{
    static constexpr DataType value = DataType::eIvec4;
};

template <>
struct TypeToEnum<Vec2u>
{
    static constexpr DataType value = DataType::eUvec2;
};

template <>
struct TypeToEnum<Vec3u>
{
    static constexpr DataType value = DataType::eUvec3;
};

template <>
struct TypeToEnum<Vec4u>
{
    static constexpr DataType value = DataType::eUvec4;
};

template <>
struct TypeToEnum<Mat2>
{
    static constexpr DataType value = DataType::eMat2;
};

template <>
struct TypeToEnum<Mat3>
{
    static constexpr DataType value = DataType::eMat3;
};

template <>
struct TypeToEnum<Mat4>
{
    static constexpr DataType value = DataType::eMat4;
};

// Get the type name as a string (useful for debugging)
[[nodiscard]] inline auto getTypeName(DataType type) -> const char*
{
    switch (type)
    {
    case DataType::eFloat:
        return "float";
    case DataType::eInt:
        return "int";
    case DataType::eUint:
        return "uint";
    case DataType::eBool:
        return "bool";
    case DataType::eVec2:
        return "vec2";
    case DataType::eVec3:
        return "vec3";
    case DataType::eVec4:
        return "vec4";
    case DataType::eIvec2:
        return "ivec2";
    case DataType::eIvec3:
        return "ivec3";
    case DataType::eIvec4:
        return "ivec4";
    case DataType::eUvec2:
        return "uvec2";
    case DataType::eUvec3:
        return "uvec3";
    case DataType::eUvec4:
        return "uvec4";
    case DataType::eMat2:
        return "mat2";
    case DataType::eMat3:
        return "mat3";
    case DataType::eMat4:
        return "mat4";
    case DataType::eTexture2D:
        return "texture2D";
    case DataType::eTextureCube:
        return "textureCube";
    case DataType::eTexture2DArray:
        return "texture2DArray";
    case DataType::eTexture3D:
        return "texture3D";
    case DataType::eSampler:
        return "sampler";
    case DataType::eBuffer:
        return "buffer";
    default:
        return "unknown";
    }
}

} // namespace aph::TypeUtils