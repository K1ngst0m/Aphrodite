enum Binding
{
    eSampledImage = 0,
    eStorageImage = 1,
    eBuffer = 2,
    eSampler = 3,
    ResourceTypeCount
};

enum Set
{
    eResource = 0,
    eHandle = 1,
};

[[vk::binding(Binding::eSampledImage, Set::eResource)]] Texture2D sampledImages[];
[[vk::binding(Binding::eStorageImage, Set::eResource)]] RWTexture2D storageImages[];
[[vk::binding(Binding::eBuffer, Set::eResource)]] StructuredBuffer<uint64_t> bufferAddressTable;
[[vk::binding(Binding::eSampler, Set::eResource)]] SamplerState samplers[];
/* [[vk::binding(Binding::eSampler, Set::eResource)]] SamplerComparisonState samplerComparisons[]; */

namespace bindless
{
    __generic<typename T>
    T getBuffer(uint id, uint64_t offset = 0)
    {
        T* ptr = (T*)bufferAddressTable[id];
        return *(ptr + offset);
    }

    __generic<typename T>
    T* getBufferPtr(uint id)
    {
        T* ptr = (T*)bufferAddressTable[id];
        return (T*)ptr;
    }

    Texture2D getSampledImage(uint id)
    {
        return sampledImages[id];
    }

    RWTexture2D getStorageImage(uint id)
    {
        return storageImages[id];
    }

    SamplerState getSampler(uint id)
    {
        return samplers[id];
    }

    float4 sample(uint textureId, uint samplerId, float2 uv)
    {
        let samp = bindless::getSampler(samplerId);
        let tex = bindless::getSampledImage(textureId);
        return tex.Sample(samp, uv);
    }

    /* SamplerComparisonState getSamplerComparisons(uint id) */
    /* { */
    /*     return samplerComparisons[id]; */
    /* } */
}

namespace bindless
{
    struct ResourceHandle
    {
        __init(uint val)
        {
            id = val;
        }
        uint id;
    };

    struct Buffer: ResourceHandle
    {
        __generic<typename T>
        T get(uint64_t offset = 0)
        {
            return getBuffer<T>(id, offset);
        }

        __generic<typename T>
        T* getPtr()
        {
            return getBufferPtr<T>(id);
        }
    };

    struct Sampler2D: ResourceHandle
    {};

    struct Texture: ResourceHandle
    {
        float4 sample(uint samplerId, float2 uv)
        {
            return sample(id, samplerId, uv);
        }

        float4 sample(Sampler2D samp, float2 uv)
        {
            return sample(id, samp.id, uv);
        }
    };
}
