#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace aph
{
// Vector Types
using Vec2 = glm::vec2; // 2D float vector
using Vec3 = glm::vec3; // 3D float vector
using Vec4 = glm::vec4; // 4D float vector

using Vec2i = glm::ivec2; // 2D integer vector
using Vec3i = glm::ivec3; // 3D integer vector
using Vec4i = glm::ivec4; // 4D integer vector

using Vec2u = glm::uvec2; // 2D unsigned integer vector
using Vec3u = glm::uvec3; // 3D unsigned integer vector
using Vec4u = glm::uvec4; // 4D unsigned integer vector

// Matrix Types
using Mat2 = glm::mat2; // 2x2 matrix
using Mat3 = glm::mat3; // 3x3 matrix
using Mat4 = glm::mat4; // 4x4 matrix

// Quaternion Type
using Quat = glm::quat;

// Create identity matrices
inline Mat4 CreateIdentity4x4()
{
    return Mat4(1.0f);
}

// Vector operations
inline float Length(const Vec2& v)
{
    return glm::length(v);
}

inline float Length(const Vec3& v)
{
    return glm::length(v);
}

inline float Length(const Vec4& v)
{
    return glm::length(v);
}

inline Vec2 Normalize(const Vec2& v)
{
    return glm::normalize(v);
}

inline Vec3 Normalize(const Vec3& v)
{
    return glm::normalize(v);
}

inline Vec4 Normalize(const Vec4& v)
{
    return glm::normalize(v);
}

inline float Dot(const Vec2& a, const Vec2& b)
{
    return glm::dot(a, b);
}

inline float Dot(const Vec3& a, const Vec3& b)
{
    return glm::dot(a, b);
}

inline float Dot(const Vec4& a, const Vec4& b)
{
    return glm::dot(a, b);
}

inline Vec3 Cross(const Vec3& a, const Vec3& b)
{
    return glm::cross(a, b);
}

// Matrix transformations
inline Mat4 Translate(const Mat4& m, const Vec3& v)
{
    return glm::translate(m, v);
}

inline Mat4 Rotate(const Mat4& m, float angle, const Vec3& axis)
{
    return glm::rotate(m, angle, axis);
}

inline Mat4 Scale(const Mat4& m, const Vec3& v)
{
    return glm::scale(m, v);
}

// Perspective and orthographic projection matrices
inline Mat4 Perspective(float fovy, float aspect, float zNear, float zFar)
{
    return glm::perspective(fovy, aspect, zNear, zFar);
}

inline Mat4 Ortho(float left, float right, float bottom, float top, float zNear, float zFar)
{
    return glm::ortho(left, right, bottom, top, zNear, zFar);
}

// Look-at matrix
inline Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up)
{
    return glm::lookAt(eye, center, up);
}

// Quaternion operations
inline Quat RotationQuat(float angle, const Vec3& axis)
{
    return glm::angleAxis(angle, axis);
}

inline Mat4 QuatToMat4(const Quat& q)
{
    return glm::mat4_cast(q);
}

inline Quat Mat4ToQuat(const Mat4& m)
{
    return glm::quat_cast(m);
}

inline Quat Conjugate(const Quat& q)
{
    return glm::conjugate(q);
}

inline Quat LookAtQuat(const Vec3& forward, const Vec3& up)
{
    // First create a rotation matrix
    Vec3 f = Normalize(forward);
    Vec3 r = Normalize(Cross(up, f));
    Vec3 u = Cross(f, r);

    Mat3 rotMatrix(r, u, f);
    return Mat4ToQuat(Mat4(rotMatrix));
}

// Constants
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 6.28318530717958647692f;
constexpr float HALF_PI = 1.57079632679489661923f;
constexpr float DEG_TO_RAD = 0.01745329251994329576f; // PI/180
constexpr float RAD_TO_DEG = 57.2957795130823208768f; // 180/PI

// Utility functions
inline float Radians(float degrees)
{
    return glm::radians(degrees);
}

inline float Degrees(float radians)
{
    return glm::degrees(radians);
}

} // namespace aph
