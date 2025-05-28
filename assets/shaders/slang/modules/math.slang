// math.slang - Common mathematical constants and utility functions

/// Common mathematical constants
namespace math
{
    // Constants
    static const float PI = 3.14159265359;
    static const float TWO_PI = 6.28318530718;
    static const float HALF_PI = 1.57079632679;
    static const float INV_PI = 0.31830988618;
    static const float EPSILON = 0.0001;
    
    // Utility functions
    float saturate(float x)
    {
        return clamp(x, 0.0, 1.0);
    }
    
    float2 saturate(float2 v)
    {
        return clamp(v, float2(0.0), float2(1.0));
    }
    
    float3 saturate(float3 v)
    {
        return clamp(v, float3(0.0), float3(1.0));
    }
    
    float4 saturate(float4 v)
    {
        return clamp(v, float4(0.0), float4(1.0));
    }
    
    // Linear interpolation
    float lerp(float a, float b, float t)
    {
        return a + t * (b - a);
    }
    
    // Smooth step function
    float smoothstep(float edge0, float edge1, float x)
    {
        float t = saturate((x - edge0) / (edge1 - edge0));
        return t * t * (3.0 - 2.0 * t);
    }
    
    // Degrees to radians conversion
    float radians(float degrees)
    {
        return degrees * (PI / 180.0);
    }
    
    // Radians to degrees conversion
    float degrees(float radians)
    {
        return radians * (180.0 / PI);
    }
    
    // Square function
    float sqr(float x)
    {
        return x * x;
    }
    
    // Cubic function
    float cube(float x)
    {
        return x * x * x;
    }
    
    // Fast approximate inverse square root
    float invSqrt(float x)
    {
        return 1.0 / sqrt(x);
    }
    
    // Spherical to Cartesian coordinate conversion
    float3 sphericalToCartesian(float radius, float theta, float phi)
    {
        float sinTheta = sin(theta);
        return float3(
            radius * sinTheta * cos(phi),
            radius * cos(theta),
            radius * sinTheta * sin(phi)
        );
    }
    
    // Cartesian to spherical coordinate conversion
    float3 cartesianToSpherical(float3 cartesian)
    {
        float radius = length(cartesian);
        if (radius < EPSILON)
            return float3(0.0, 0.0, 0.0);
            
        return float3(
            radius,
            acos(cartesian.y / radius),
            atan2(cartesian.z, cartesian.x)
        );
    }
    
    // Linear to sRGB conversion
    float3 linearToSRGB(float3 color)
    {
        float3 cutoff = float3(color.x > 0.0031308 ? 1.0 : 0.0, 
                             color.y > 0.0031308 ? 1.0 : 0.0, 
                             color.z > 0.0031308 ? 1.0 : 0.0);
        return lerp(12.92 * color, 1.055 * pow(color, float3(1.0/2.4)) - 0.055, cutoff);
    }
    
    // sRGB to linear conversion
    float3 sRGBToLinear(float3 srgbColor)
    {
        float3 cutoff = float3(srgbColor.x > 0.04045 ? 1.0 : 0.0,
                             srgbColor.y > 0.04045 ? 1.0 : 0.0,
                             srgbColor.z > 0.04045 ? 1.0 : 0.0);
        return lerp(srgbColor / 12.92, pow((srgbColor + 0.055) / 1.055, float3(2.4)), cutoff);
    }
} 