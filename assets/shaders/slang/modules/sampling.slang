// sampling.slang - Sampling utilities for various techniques

import math;

namespace sampling
{
    // Low-discrepancy sequence (Hammersley)
    float radicalInverse_VdC(uint bits) 
    {
        bits = (bits << 16u) | (bits >> 16u);
        bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
        bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
        bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
        bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
        return float(bits) * 2.3283064365386963e-10; // / 0x100000000
    }

    // Hammersley sequence for sample generation
    float2 hammersley(uint i, uint N)
    {
        return float2(float(i)/float(N), radicalInverse_VdC(i));
    }
    
    // Importance sampling GGX for specular reflection
    float3 importanceSampleGGX(float2 Xi, float3 N, float roughness)
    {
        float a = roughness * roughness;
        
        float phi = 2.0 * math::PI * Xi.x;
        float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
        float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
        
        // From spherical coordinates to cartesian coordinates
        float3 H;
        H.x = cos(phi) * sinTheta;
        H.y = sin(phi) * sinTheta;
        H.z = cosTheta;
        
        // From tangent-space to world-space
        float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
        float3 tangent = normalize(cross(up, N));
        float3 bitangent = cross(N, tangent);
        
        float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
        return normalize(sampleVec);
    }
    
    // Generate prefiltered sample direction
    float3 prefilteredSampleDirection(float2 Xi, float3 N, float roughness)
    {
        float3 H = importanceSampleGGX(Xi, N, roughness);
        float3 V = N;  // Assume view direction is along the normal for simplicity
        return reflect(-V, H);
    }
    
    // Poisson disk sampling positions
    static const float2 poissonDisk[16] = {
        float2(-0.94201624, -0.39906216),
        float2(0.94558609, -0.76890725),
        float2(-0.094184101, -0.92938870),
        float2(0.34495938, 0.29387760),
        float2(-0.91588581, 0.45771432),
        float2(-0.81544232, -0.87912464),
        float2(-0.38277543, 0.27676845),
        float2(0.97484398, 0.75648379),
        float2(0.44323325, -0.97511554),
        float2(0.53742981, -0.47373420),
        float2(-0.26496911, -0.41893023),
        float2(0.79197514, 0.19090188),
        float2(-0.24188840, 0.99706507),
        float2(-0.81409955, 0.91437590),
        float2(0.19984126, 0.78641367),
        float2(0.14383161, -0.14100790)
    };
    
    // Get random rotation for poisson disk sampling
    float2 poissonRotation(float2 uv, float seed)
    {
        float noise = frac(sin(dot(uv, float2(12.9898, 78.233) * seed)) * 43758.5453);
        float angle = 2.0 * math::PI * noise;
        return float2(cos(angle), sin(angle));
    }
    
    // Rotate 2D vector
    float2 rotate(float2 v, float2 rot)
    {
        return float2(v.x * rot.x - v.y * rot.y, v.x * rot.y + v.y * rot.x);
    }
    
    // Golden ratio spiral points on a disk
    float2 goldenRatioSpiral(uint i, uint n, float radius)
    {
        const float goldenAngle = 2.4;
        float theta = float(i) * goldenAngle;
        float r = radius * sqrt(float(i) / float(n));
        return float2(r * cos(theta), r * sin(theta));
    }
    
    // Cosine weighted hemisphere sampling
    float3 cosineWeightedHemisphere(float2 uv, float3 normal)
    {
        // Cosine weighted distribution
        float phi = 2.0 * math::PI * uv.x;
        float cosTheta = sqrt(uv.y);
        float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
        
        // Convert to cartesian coordinates
        float3 localDir = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
        
        // Create coordinate system around normal
        float3 up = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
        float3 tangent = normalize(cross(up, normal));
        float3 bitangent = cross(normal, tangent);
        
        // Transform from local to world
        return normalize(tangent * localDir.x + bitangent * localDir.y + normal * localDir.z);
    }
    
    // Multiple Importance Sampling weight calculation
    float balanceHeuristic(float pdfA, float pdfB)
    {
        return pdfA / (pdfA + pdfB);
    }
    
    float powerHeuristic(float pdfA, float pdfB, float beta = 2.0)
    {
        float a = pow(pdfA, beta);
        float b = pow(pdfB, beta);
        return a / (a + b);
    }
    
    // Stratified sampling positions in a square
    float2 stratifiedSample(uint x, uint y, uint width, uint height, float2 randomOffset)
    {
        return float2((float(x) + randomOffset.x) / float(width), 
                      (float(y) + randomOffset.y) / float(height));
    }
    
    // Blue noise texture sampling
    float sampleBlueNoise(Texture2D blueNoiseTex, SamplerState samplerState, uint2 pixelCoord, uint frameIndex)
    {
        uint2 texDim;
        blueNoiseTex.GetDimensions(texDim.x, texDim.y);
        uint2 offset = uint2(frameIndex * 16, 0); // Assuming 16 frame patterns
        uint2 wrappedCoord = (pixelCoord + offset) % texDim;
        return blueNoiseTex.Load(int3(wrappedCoord, 0)).r;
    }
} 