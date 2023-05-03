#ifndef GPU_RESOURCE_H_
#define GPU_RESOURCE_H_

#include "common/common.h"

namespace aph
{
enum class BufferDomain
{
    Device,                            // Device local. Probably not visible from CPU.
    LinkedDeviceHost,                  // On desktop, directly mapped VRAM over PCI.
    LinkedDeviceHostPreferDevice,      // Prefer device local of host visible.
    Host,                              // Host-only, needs to be synced to GPU. Might be device local as well on iGPUs.
    CachedHost,                        //
    CachedCoherentHostPreferCoherent,  // Aim for both cached and coherent, but prefer COHERENT
    CachedCoherentHostPreferCached,    // Aim for both cached and coherent, but prefer CACHED
};

enum class ImageDomain
{
    Device,
    Transient,
    LinearHostCached,
    LinearHost
};

enum class ShaderStage
{
    NA  = 0,
    VS  = 1,
    TCS = 2,
    TES = 3,
    GS  = 4,
    FS  = 5,
    CS  = 6,
    TS  = 7,
    MS  = 8,
};

enum class ResourceType
{
    UNDEFINED             = 0,
    SAMPLER               = 1,
    SAMPLED_IMAGE         = 2,
    COMBINE_SAMPLER_IMAGE = 3,
    STORAGE_IMAGE         = 4,
    UNIFORM_BUFFER        = 5,
    STORAGE_BUFFER        = 6,
};

struct Extent3D
{
    uint32_t width  = {0};
    uint32_t height = {0};
    uint32_t depth  = {0};
};

struct DummyCreateInfo
{
    uint32_t typeId;
};

template <typename T_Handle, typename T_CreateInfo = DummyCreateInfo>
class ResourceHandle
{
public:
    ResourceHandle()
    {
        if constexpr(std::is_same_v<T_CreateInfo, DummyCreateInfo>)
        {
            m_createInfo.typeId = typeid(T_Handle).hash_code();
        }
    }
    operator T_Handle() { return m_handle; }
    operator T_Handle&() { return m_handle; }
    operator T_Handle&() const { return m_handle; }

    T_Handle&       getHandle() { return m_handle; }
    const T_Handle& getHandle() const { return m_handle; }
    T_CreateInfo&   getCreateInfo() { return m_createInfo; }

protected:
    T_Handle     m_handle     = {};
    T_CreateInfo m_createInfo = {};
};

}  // namespace aph

#endif  // RESOURCE_H_
