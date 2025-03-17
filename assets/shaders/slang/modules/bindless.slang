enum Binding
{
    eImage = 0,
    eBuffer = 1,
    eSampler = 2,
    ResourceTypeCount
};

enum Set
{
    eResource = 0,
    eHandle = 1,
};

[[allow("parameterBindingsOverlap")]]
[[vk::binding(Binding::eImage, Set::eResource)]] __DynamicResource<__DynamicResourceKind.General> images[];
[[allow("parameterBindingsOverlap")]]
[[vk::binding(Binding::eBuffer, Set::eResource)]] StructuredBuffer<uint64_t> bufferAddressTable;
[[allow("parameterBindingsOverlap")]]
[[vk::binding(Binding::eSampler, Set::eResource)]] SamplerState samplers[];

namespace bindless
{
    __generic<typename T>
    export T getBuffer(uint id, uint64_t offset = 0)
    {
        T* ptr = (T*)bufferAddressTable[id];
        return *(ptr + offset);
    }

    __generic<typename T>
    export T* getBufferPtr(uint id)
    {
        T* ptr = (T*)bufferAddressTable[id];
        return (T*)ptr;
    }

    export Texture2D getImage(uint id)
    {
        return images[id];
    }

    export SamplerState getSampler(uint id)
    {
        return samplers[id];
    }

    export float4 sample(uint textureId, uint samplerId, float2 uv)
    {
        let samp = getSampler(samplerId);
        let tex = getImage(textureId);
        return tex.Sample(samp, uv);
    }
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
