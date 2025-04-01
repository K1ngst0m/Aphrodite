#include "math.h"

namespace aph::utils
{
// Custom string conversion functions
std::string ToString(const Vec2& v);
std::string ToString(const Vec3& v);
std::string ToString(const Vec4& v);
std::string ToString(const Vec2i& v);
std::string ToString(const Vec3i& v);
std::string ToString(const Vec4i& v);
std::string ToString(const Vec2u& v);
std::string ToString(const Vec3u& v);
std::string ToString(const Vec4u& v);
std::string ToString(const Mat2& m);
std::string ToString(const Mat3& m);
std::string ToString(const Mat4& m);
std::string ToString(const Quat& q);

// Stream operators for easy printing
std::ostream& operator<<(std::ostream& os, const Vec2& v);
std::ostream& operator<<(std::ostream& os, const Vec3& v);
std::ostream& operator<<(std::ostream& os, const Vec4& v);
std::ostream& operator<<(std::ostream& os, const Vec2i& v);
std::ostream& operator<<(std::ostream& os, const Vec3i& v);
std::ostream& operator<<(std::ostream& os, const Vec4i& v);
std::ostream& operator<<(std::ostream& os, const Vec2u& v);
std::ostream& operator<<(std::ostream& os, const Vec3u& v);
std::ostream& operator<<(std::ostream& os, const Vec4u& v);
std::ostream& operator<<(std::ostream& os, const Mat2& m);
std::ostream& operator<<(std::ostream& os, const Mat3& m);
std::ostream& operator<<(std::ostream& os, const Mat4& m);
std::ostream& operator<<(std::ostream& os, const Quat& q);

} // namespace aph::utils
