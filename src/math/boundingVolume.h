#pragma once

#include "math.h"
#include <limits>

namespace aph
{

// Axis-aligned bounding box
struct BoundingBox
{
    Vec3 min = Vec3{std::numeric_limits<float>::max()};
    Vec3 max = Vec3{-std::numeric_limits<float>::max()};

    // Default constructor creates an "empty" AABB
    BoundingBox() = default;

    // Create from min/max points
    BoundingBox(const Vec3& minPoint, const Vec3& maxPoint)
        : min(minPoint)
        , max(maxPoint)
    {
    }

    // Create from center and half-extents
    static BoundingBox FromCenterAndExtent(const Vec3& center, const Vec3& halfExtent)
    {
        return BoundingBox(center - halfExtent, center + halfExtent);
    }

    // Check if the box is valid (i.e., has been initialized with actual points)
    bool isValid() const
    {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    // Get the center of the bounding box
    Vec3 getCenter() const
    {
        return (min + max) * 0.5f;
    }

    // Get the extents (size) of the bounding box
    Vec3 getExtent() const
    {
        return max - min;
    }

    // Get the half-extents (half-size) of the bounding box
    Vec3 getHalfExtent() const
    {
        return getExtent() * 0.5f;
    }

    // Extend the bounding box to include the given point
    void extend(const Vec3& point)
    {
        min = Min(min, point);
        max = Max(max, point);
    }

    // Extend the bounding box to include another bounding box
    void extend(const BoundingBox& other)
    {
        if (other.isValid())
        {
            min = Min(min, other.min);
            max = Max(max, other.max);
        }
    }

    // Check if a point is inside the bounding box
    bool contains(const Vec3& point) const
    {
        return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y && point.z >= min.z &&
               point.z <= max.z;
    }

    // Transform the bounding box by a matrix
    BoundingBox transform(const Mat4& matrix) const
    {
        // Start with an empty box
        BoundingBox result;

        // Create the 8 corners of the box
        Vec3 corners[8] = {Vec3(min.x, min.y, min.z), Vec3(max.x, min.y, min.z), Vec3(min.x, max.y, min.z),
                           Vec3(max.x, max.y, min.z), Vec3(min.x, min.y, max.z), Vec3(max.x, min.y, max.z),
                           Vec3(min.x, max.y, max.z), Vec3(max.x, max.y, max.z)};

        // Transform each corner and extend the result
        for (int i = 0; i < 8; i++)
        {
            Vec4 transformedCorner = matrix * Vec4(corners[i], 1.0f);
            result.extend(Vec3(transformedCorner) / transformedCorner.w);
        }

        return result;
    }
};

// Bounding Sphere
struct BoundingSphere
{
    Vec3 center = Vec3(0.0f);
    float radius = 0.0f;

    // Default constructor
    BoundingSphere() = default;

    // Create from center and radius
    BoundingSphere(const Vec3& center, float radius)
        : center(center)
        , radius(radius)
    {
    }

    // Create a bounding sphere from an AABB
    static BoundingSphere FromBoundingBox(const BoundingBox& box)
    {
        BoundingSphere sphere;
        sphere.center = box.getCenter();
        sphere.radius = Length(box.getHalfExtent());
        return sphere;
    }

    // Check if the sphere contains a point
    bool contains(const Vec3& point) const
    {
        float distSq = Distance2(center, point);
        return distSq <= (radius * radius);
    }

    // Extend the sphere to include a point
    void extend(const Vec3& point)
    {
        if (radius == 0.0f)
        {
            // First point, just set center and zero radius
            center = point;
            radius = 0.0f;
            return;
        }

        float dist = Distance(center, point);
        if (dist > radius)
        {
            // Point is outside the sphere, extend
            float newRadius = (radius + dist) * 0.5f;
            float k = (newRadius - radius) / dist;
            radius = newRadius;
            center = center + k * (point - center);
        }
    }

    // Extend the sphere to include another sphere
    void extend(const BoundingSphere& other)
    {
        if (other.radius == 0.0f)
            return;

        if (radius == 0.0f)
        {
            center = other.center;
            radius = other.radius;
            return;
        }

        // Find the distance between centers
        float dist = Distance(center, other.center);

        // If one sphere contains the other, return the larger one
        if (dist + other.radius <= radius)
            return; // This sphere contains the other

        if (dist + radius <= other.radius)
        {
            // Other sphere contains this one
            center = other.center;
            radius = other.radius;
            return;
        }

        // Spheres overlap or are separate, merge them
        float newRadius = (radius + dist + other.radius) * 0.5f;
        Vec3 dir = other.center - center;
        if (dist > 0.0f)
            dir = dir / dist; // Normalize

        center = center + dir * (newRadius - radius);
        radius = newRadius;
    }

    // Transform the sphere by a matrix
    BoundingSphere transform(const Mat4& matrix) const
    {
        // Extract the scale from the matrix (approximation)
        Vec3 scale = Vec3(Length(Vec3(matrix[0][0], matrix[0][1], matrix[0][2])),
                          Length(Vec3(matrix[1][0], matrix[1][1], matrix[1][2])),
                          Length(Vec3(matrix[2][0], matrix[2][1], matrix[2][2])));

        // Use the maximum scale
        float maxScale = Max(Max(scale.x, scale.y), scale.z);

        // Transform the center
        Vec4 transformedCenter = matrix * Vec4(center, 1.0f);

        return BoundingSphere(Vec3(transformedCenter) / transformedCenter.w, radius * maxScale);
    }
};

// Frustum for view culling
struct Frustum
{
    enum Plane
    {
        Left = 0,
        Right,
        Bottom,
        Top,
        Near,
        Far,
        PlaneCount
    };

    Vec4 planes[PlaneCount]; // Planes in ax + by + cz + d = 0 form

    // Default constructor
    Frustum() = default;

    // Construct from a view-projection matrix
    Frustum(const Mat4& viewProj)
    {
        setFromMatrix(viewProj);
    }

    // Set frustum planes from a view-projection matrix
    void setFromMatrix(const Mat4& viewProj)
    {
        // Left plane
        planes[Left].x = viewProj[0][3] + viewProj[0][0];
        planes[Left].y = viewProj[1][3] + viewProj[1][0];
        planes[Left].z = viewProj[2][3] + viewProj[2][0];
        planes[Left].w = viewProj[3][3] + viewProj[3][0];

        // Right plane
        planes[Right].x = viewProj[0][3] - viewProj[0][0];
        planes[Right].y = viewProj[1][3] - viewProj[1][0];
        planes[Right].z = viewProj[2][3] - viewProj[2][0];
        planes[Right].w = viewProj[3][3] - viewProj[3][0];

        // Bottom plane
        planes[Bottom].x = viewProj[0][3] + viewProj[0][1];
        planes[Bottom].y = viewProj[1][3] + viewProj[1][1];
        planes[Bottom].z = viewProj[2][3] + viewProj[2][1];
        planes[Bottom].w = viewProj[3][3] + viewProj[3][1];

        // Top plane
        planes[Top].x = viewProj[0][3] - viewProj[0][1];
        planes[Top].y = viewProj[1][3] - viewProj[1][1];
        planes[Top].z = viewProj[2][3] - viewProj[2][1];
        planes[Top].w = viewProj[3][3] - viewProj[3][1];

        // Near plane
        planes[Near].x = viewProj[0][3] + viewProj[0][2];
        planes[Near].y = viewProj[1][3] + viewProj[1][2];
        planes[Near].z = viewProj[2][3] + viewProj[2][2];
        planes[Near].w = viewProj[3][3] + viewProj[3][2];

        // Far plane
        planes[Far].x = viewProj[0][3] - viewProj[0][2];
        planes[Far].y = viewProj[1][3] - viewProj[1][2];
        planes[Far].z = viewProj[2][3] - viewProj[2][2];
        planes[Far].w = viewProj[3][3] - viewProj[3][2];

        // Normalize all planes
        for (int i = 0; i < PlaneCount; ++i)
        {
            float invLen = 1.0f / Length(Vec3(planes[i]));
            planes[i] *= invLen;
        }
    }

    // Test if a point is inside the frustum
    bool contains(const Vec3& point) const
    {
        for (int i = 0; i < PlaneCount; ++i)
        {
            if (Dot(Vec3(planes[i]), point) + planes[i].w < 0.0f)
                return false;
        }
        return true;
    }

    // Test if a sphere is inside or intersects the frustum
    bool intersects(const BoundingSphere& sphere) const
    {
        for (int i = 0; i < PlaneCount; ++i)
        {
            float dist = Dot(Vec3(planes[i]), sphere.center) + planes[i].w;
            if (dist < -sphere.radius)
                return false;
        }
        return true;
    }

    // Test if a box is inside or intersects the frustum
    bool intersects(const BoundingBox& box) const
    {
        for (int i = 0; i < PlaneCount; ++i)
        {
            // Get the positive vertex relative to the plane normal
            Vec3 p = box.min;
            if (planes[i].x >= 0.0f)
                p.x = box.max.x;
            if (planes[i].y >= 0.0f)
                p.y = box.max.y;
            if (planes[i].z >= 0.0f)
                p.z = box.max.z;

            // If this point is outside, the entire box is outside
            if (Dot(Vec3(planes[i]), p) + planes[i].w < 0.0f)
                return false;
        }
        return true;
    }
};

} // namespace aph
