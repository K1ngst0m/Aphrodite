// transform.slang - Transformation and space conversion utilities

import math;

namespace transform
{
    // Generate tangent-bitangent-normal (TBN) matrix
    float3x3 createTBN(float3 normal, float3 tangent, float3 bitangent)
    {
        return float3x3(
            normalize(tangent),
            normalize(bitangent),
            normalize(normal)
        );
    }
    
    // Generate TBN from normal and tangent with handedness
    float3x3 createTBN(float3 normal, float3 tangent, float tangentHandedness)
    {
        float3 N = normalize(normal);
        float3 T = normalize(tangent - N * dot(tangent, N)); // Gram-Schmidt orthogonalization
        float3 B = cross(N, T) * tangentHandedness;
        return float3x3(T, B, N);
    }
    
    // Transform direction from tangent space to world space
    float3 tangentToWorld(float3 tangentDir, float3x3 TBN)
    {
        return mul(tangentDir, TBN);
    }
    
    // Transform direction from world space to tangent space
    float3 worldToTangent(float3 worldDir, float3x3 TBN)
    {
        return float3(
            dot(worldDir, TBN[0]),
            dot(worldDir, TBN[1]),
            dot(worldDir, TBN[2])
        );
    }
    
    // Create a look-at matrix
    float4x4 lookAt(float3 eye, float3 target, float3 up)
    {
        float3 zaxis = normalize(eye - target);
        float3 xaxis = normalize(cross(up, zaxis));
        float3 yaxis = cross(zaxis, xaxis);
        
        return float4x4(
            float4(xaxis.x, yaxis.x, zaxis.x, 0),
            float4(xaxis.y, yaxis.y, zaxis.y, 0),
            float4(xaxis.z, yaxis.z, zaxis.z, 0),
            float4(-dot(xaxis, eye), -dot(yaxis, eye), -dot(zaxis, eye), 1)
        );
    }
    
    // Create a perspective projection matrix
    float4x4 perspective(float fovY, float aspect, float zNear, float zFar)
    {
        float f = 1.0 / tan(fovY * 0.5);
        float d = zFar - zNear;
        
        return float4x4(
            float4(f / aspect, 0, 0, 0),
            float4(0, f, 0, 0),
            float4(0, 0, zFar / d, 1),
            float4(0, 0, -zNear * zFar / d, 0)
        );
    }
    
    // Create a translation matrix
    float4x4 translation(float3 offset)
    {
        return float4x4(
            float4(1, 0, 0, 0),
            float4(0, 1, 0, 0),
            float4(0, 0, 1, 0),
            float4(offset.x, offset.y, offset.z, 1)
        );
    }
    
    // Create a rotation matrix from Euler angles (in radians)
    float4x4 rotation(float3 eulerAngles)
    {
        float sx = sin(eulerAngles.x);
        float cx = cos(eulerAngles.x);
        float sy = sin(eulerAngles.y);
        float cy = cos(eulerAngles.y);
        float sz = sin(eulerAngles.z);
        float cz = cos(eulerAngles.z);
        
        // Rotation order: Z * Y * X (roll, pitch, yaw)
        return float4x4(
            float4(cy * cz, cy * sz, -sy, 0),
            float4(sx * sy * cz - cx * sz, sx * sy * sz + cx * cz, sx * cy, 0),
            float4(cx * sy * cz + sx * sz, cx * sy * sz - sx * cz, cx * cy, 0),
            float4(0, 0, 0, 1)
        );
    }
    
    // Create a scale matrix
    float4x4 scale(float3 factors)
    {
        return float4x4(
            float4(factors.x, 0, 0, 0),
            float4(0, factors.y, 0, 0),
            float4(0, 0, factors.z, 0),
            float4(0, 0, 0, 1)
        );
    }
    
    // Extract translation from a transform matrix
    float3 getTranslation(float4x4 matrix)
    {
        return float3(matrix[3][0], matrix[3][1], matrix[3][2]);
    }
    
    // Extract scale from a transform matrix (approximate)
    float3 getScale(float4x4 matrix)
    {
        return float3(
            length(float3(matrix[0][0], matrix[0][1], matrix[0][2])),
            length(float3(matrix[1][0], matrix[1][1], matrix[1][2])),
            length(float3(matrix[2][0], matrix[2][1], matrix[2][2]))
        );
    }
    
    // Convert from clip space to screen space
    float2 clipToScreen(float4 clipPos, float2 screenSize)
    {
        float2 ndcPos = clipPos.xy / clipPos.w;
        return (ndcPos * 0.5 + 0.5) * screenSize;
    }
    
    // Convert screen position to world ray
    float3 screenToWorldRay(float2 screenPos, float2 screenSize, float4x4 inverseViewProj)
    {
        // Convert to NDC space
        float2 ndcPos = screenPos / screenSize * 2.0 - 1.0;
        ndcPos.y = -ndcPos.y; // Flip Y for Vulkan/DirectX
        
        // Convert to homogeneous clip space
        float4 clipPos = float4(ndcPos, 1.0, 1.0);
        
        // Transform to world space
        float4 worldPos = mul(clipPos, inverseViewProj);
        worldPos /= worldPos.w;
        
        // Calculate ray from eye to world position
        float3 worldCamPos = getTranslation(inverseViewProj);
        return normalize(worldPos.xyz - worldCamPos);
    }
    
    // Create a world-space plane from a point and normal
    float4 createPlane(float3 point, float3 normal)
    {
        normal = normalize(normal);
        return float4(normal, -dot(normal, point));
    }
    
    // Ray-plane intersection
    bool rayPlaneIntersection(float3 rayOrigin, float3 rayDir, float4 plane, out float t, out float3 intersection)
    {
        float3 planeNormal = plane.xyz;
        float planeDistance = plane.w;
        
        float denominator = dot(rayDir, planeNormal);
        
        // Check if ray is parallel to plane
        if (abs(denominator) < math::EPSILON)
        {
            t = 0;
            intersection = float3(0, 0, 0);
            return false;
        }
        
        t = -(dot(rayOrigin, planeNormal) + planeDistance) / denominator;
        
        // Check if intersection is behind the ray
        if (t < 0)
        {
            intersection = float3(0, 0, 0);
            return false;
        }
        
        intersection = rayOrigin + rayDir * t;
        return true;
    }
    
    // Transform a point from one space to another
    float3 transformPoint(float3 point, float4x4 matrix)
    {
        float4 homogeneous = mul(float4(point, 1.0), matrix);
        return homogeneous.xyz / homogeneous.w;
    }
    
    // Transform a direction from one space to another
    float3 transformDirection(float3 dir, float4x4 matrix)
    {
        return normalize(mul(float4(dir, 0.0), matrix).xyz);
    }
    
    // Transform a normal (uses inverse transpose of upper 3x3 matrix)
    float3 transformNormal(float3 normal, float4x4 matrix)
    {
        // Extract the 3x3 part of the matrix and compute its inverse transpose
        float3x3 upperLeft = float3x3(
            matrix[0].xyz,
            matrix[1].xyz,
            matrix[2].xyz
        );
        
        // Simple approximation for normal transformation
        // (This works for rigid transformations, but not for non-uniform scaling)
        return normalize(mul(normal, upperLeft));
    }
} 