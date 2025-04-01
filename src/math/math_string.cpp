#include "math_string.h"

#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>

namespace aph::utils
{
template <typename T>
inline std::string FormatFloat(const T& value, int precision = 6)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    return ss.str();
}

std::string ToString(const Vec2& v)
{
    std::stringstream ss;
    ss << "Vec2(" << FormatFloat(v.x) << ", " << FormatFloat(v.y) << ")";
    return ss.str();
}

std::string ToString(const Vec3& v)
{
    std::stringstream ss;
    ss << "Vec3(" << FormatFloat(v.x) << ", " << FormatFloat(v.y) << ", " << FormatFloat(v.z) << ")";
    return ss.str();
}

std::string ToString(const Vec4& v)
{
    std::stringstream ss;
    ss << "Vec4(" << FormatFloat(v.x) << ", " << FormatFloat(v.y) << ", " << FormatFloat(v.z) << ", "
       << FormatFloat(v.w) << ")";
    return ss.str();
}

std::string ToString(const Vec2i& v)
{
    std::stringstream ss;
    ss << "Vec2i(" << v.x << ", " << v.y << ")";
    return ss.str();
}

std::string ToString(const Vec3i& v)
{
    std::stringstream ss;
    ss << "Vec3i(" << v.x << ", " << v.y << ", " << v.z << ")";
    return ss.str();
}

std::string ToString(const Vec4i& v)
{
    std::stringstream ss;
    ss << "Vec4i(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
    return ss.str();
}

std::string ToString(const Vec2u& v)
{
    std::stringstream ss;
    ss << "Vec2u(" << v.x << ", " << v.y << ")";
    return ss.str();
}

std::string ToString(const Vec3u& v)
{
    std::stringstream ss;
    ss << "Vec3u(" << v.x << ", " << v.y << ", " << v.z << ")";
    return ss.str();
}

std::string ToString(const Vec4u& v)
{
    std::stringstream ss;
    ss << "Vec4u(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
    return ss.str();
}

std::string ToString(const Mat2& m)
{
    std::stringstream ss;
    ss << "Mat2(\n";
    for (int row = 0; row < 2; ++row)
    {
        ss << "  ";
        for (int col = 0; col < 2; ++col)
        {
            ss << FormatFloat(m[col][row]);
            if (col < 1)
                ss << ", ";
        }
        ss << "\n";
    }
    ss << ")";
    return ss.str();
}

std::string ToString(const Mat3& m)
{
    std::stringstream ss;
    ss << "Mat3(\n";
    for (int row = 0; row < 3; ++row)
    {
        ss << "  ";
        for (int col = 0; col < 3; ++col)
        {
            ss << FormatFloat(m[col][row]);
            if (col < 2)
                ss << ", ";
        }
        ss << "\n";
    }
    ss << ")";
    return ss.str();
}

std::string ToString(const Mat4& m)
{
    std::stringstream ss;
    ss << "Mat4(\n";
    for (int row = 0; row < 4; ++row)
    {
        ss << "  ";
        for (int col = 0; col < 4; ++col)
        {
            ss << FormatFloat(m[col][row]);
            if (col < 3)
                ss << ", ";
        }
        ss << "\n";
    }
    ss << ")";
    return ss.str();
}

std::string ToString(const Quat& q)
{
    std::stringstream ss;
    ss << "Quat(" << FormatFloat(q.w) << ", " << FormatFloat(q.x) << ", " << FormatFloat(q.y) << ", "
       << FormatFloat(q.z) << ")";
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const Vec2& v)
{
    os << ToString(v);
    return os;
}

std::ostream& operator<<(std::ostream& os, const Vec3& v)
{
    os << ToString(v);
    return os;
}

std::ostream& operator<<(std::ostream& os, const Vec4& v)
{
    os << ToString(v);
    return os;
}

std::ostream& operator<<(std::ostream& os, const Vec2i& v)
{
    os << ToString(v);
    return os;
}

std::ostream& operator<<(std::ostream& os, const Vec3i& v)
{
    os << ToString(v);
    return os;
}

std::ostream& operator<<(std::ostream& os, const Vec4i& v)
{
    os << ToString(v);
    return os;
}

std::ostream& operator<<(std::ostream& os, const Vec2u& v)
{
    os << ToString(v);
    return os;
}

std::ostream& operator<<(std::ostream& os, const Vec3u& v)
{
    os << ToString(v);
    return os;
}

std::ostream& operator<<(std::ostream& os, const Vec4u& v)
{
    os << ToString(v);
    return os;
}

std::ostream& operator<<(std::ostream& os, const Mat2& m)
{
    os << ToString(m);
    return os;
}

std::ostream& operator<<(std::ostream& os, const Mat3& m)
{
    os << ToString(m);
    return os;
}

std::ostream& operator<<(std::ostream& os, const Mat4& m)
{
    os << ToString(m);
    return os;
}

std::ostream& operator<<(std::ostream& os, const Quat& q)
{
    os << ToString(q);
    return os;
}
} // namespace aph::utils
