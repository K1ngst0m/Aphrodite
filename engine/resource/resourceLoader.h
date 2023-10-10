#ifndef RES_LOADER_H_
#define RES_LOADER_H_

#include "common/common.h"
#include "api/vulkan/device.h"

namespace aph
{

struct ResourceLoaderCreateInfo
{
    // TODO
    bool        isMultiThreads = false;
    vk::Device* pDevice        = {};
};

enum class ImageContainerType
{
    Default = 0,
    Dds,
    Ktx,
    Png,
    Jpg,
};

struct ImageLoadInfo
{
    std::variant<std::string, ImageInfo> data;
    ImageContainerType                   containerType = {ImageContainerType::Default};
    vk::ImageCreateInfo*                 pCreateInfo   = {};
    vk::Image*                           pImage        = {};
};

struct BufferLoadInfo
{
    void*                data;
    vk::BufferCreateInfo createInfo = {};
    vk::Buffer**         ppBuffer   = {};
};

class ResourceLoader
{
public:
    ResourceLoader(const ResourceLoaderCreateInfo& createInfo);

    ~ResourceLoader() = default;

    void loadImages(ImageLoadInfo& info);
    void loadBuffers(BufferLoadInfo& info);

private:
    ResourceLoaderCreateInfo m_createInfo;
    vk::Device*              m_pDevice;
};
}  // namespace aph

#endif
