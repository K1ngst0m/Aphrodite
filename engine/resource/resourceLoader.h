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
};

struct BufferLoadInfo
{
    void*                data       = {};
    vk::BufferCreateInfo createInfo = {};
};

struct ShaderLoadInfo
{
    std::variant<std::string, std::vector<uint32_t>> data;
    std::string                                      entryPoint = "main";
    std::vector<ShaderMacro>                         macros;
};

class ResourceLoader
{
public:
    ResourceLoader(const ResourceLoaderCreateInfo& createInfo);

    ~ResourceLoader();

    void load(const ImageLoadInfo& info, vk::Image** ppImage);
    void load(const BufferLoadInfo& info, vk::Buffer** ppBuffer);
    void load(const ShaderLoadInfo& info, vk::Shader** ppShader);

    void cleanup();

private:
    ResourceLoaderCreateInfo m_createInfo;
    vk::Device*              m_pDevice = {};

private:
    std::unordered_map<std::string, std::unique_ptr<vk::Shader>> m_shaderModuleCaches = {};
    uuid::UUIDGenerator<std::mt19937_64>                         m_uuidGenerator      = {};
    std::unordered_map<std::string, std::string>                 m_shaderUUIDMap      = {};
};
}  // namespace aph

#endif
