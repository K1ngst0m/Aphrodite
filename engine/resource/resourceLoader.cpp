#include "resourceLoader.h"
#include "api/vulkan/device.h"
#include "tinyimageformat.h"
#include "tinyktx.h"

namespace
{
inline bool loadKTX(const std::filesystem::path& path, aph::vk::ImageCreateInfo* pOutCI, std::vector<uint8_t>& data)
{
    if(!std::filesystem::exists(path))
    {
        return false;
    }

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    ssize_t       ktxDataSize = file.tellg();

    if(ktxDataSize > UINT32_MAX)
    {
        return false;
    }

    file.seekg(0);

    TinyKtx_Callbacks callbacks{[](void* user, char const* msg) { CM_LOG_ERR("%s", msg); },
                                [](void* user, size_t size) { return malloc(size); },
                                [](void* user, void* memory) { free(memory); },
                                [](void* user, void* buffer, size_t byteCount) {
                                    auto ifs = static_cast<std::ifstream*>(user);
                                    ifs->read((char*)buffer, byteCount);
                                    return (size_t)ifs->gcount();
                                },
                                [](void* user, int64_t offset) {
                                    auto ifs = static_cast<std::ifstream*>(user);
                                    ifs->seekg(offset);
                                    return !ifs->fail();
                                },
                                [](void* user) { return (int64_t)(static_cast<std::ifstream*>(user)->tellg()); }};

    TinyKtx_ContextHandle ctx        = TinyKtx_CreateContext(&callbacks, &file);
    bool                  headerOkay = TinyKtx_ReadHeader(ctx);
    if(!headerOkay)
    {
        TinyKtx_DestroyContext(ctx);
        return false;
    }

    aph::vk::ImageCreateInfo& textureCI = *pOutCI;
    textureCI.extent                    = {
                           .width  = TinyKtx_Width(ctx),
                           .height = TinyKtx_Height(ctx),
                           .depth  = std::max(1U, TinyKtx_Depth(ctx)),
    };
    textureCI.arraySize = std::max(1U, TinyKtx_ArraySlices(ctx));
    textureCI.mipLevels = std::max(1U, TinyKtx_NumberOfMipmaps(ctx));
    textureCI.format = (VkFormat)TinyImageFormat_ToVkFormat(TinyImageFormat_FromTinyKtxFormat(TinyKtx_GetFormat(ctx)));
    // textureCI.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    textureCI.sampleCount = 1;

    if(textureCI.format == VK_FORMAT_UNDEFINED)
    {
        TinyKtx_DestroyContext(ctx);
        return false;
    }

    if(TinyKtx_IsCubemap(ctx))
    {
        textureCI.arraySize *= 6;
        // textureCI.descriptorType |= VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    }

    TinyKtx_DestroyContext(ctx);

    return true;
}

inline bool loadPNGJPG(const std::filesystem::path& path, aph::vk::ImageCreateInfo* pOutCI, std::vector<uint8_t>& data)
{
    auto img = aph::utils::loadImageFromFile(path.c_str());

    if(img == nullptr)
    {
        return false;
    }

    aph::vk::ImageCreateInfo& textureCI = *pOutCI;

    textureCI.extent = {
        .width  = img->width,
        .height = img->height,
        .depth  = 1,
    };

    return true;
}
}  // namespace

namespace aph
{

ResourceLoader::ResourceLoader(const ResourceLoaderCreateInfo& createInfo) :
    m_createInfo(createInfo),
    m_pDevice(createInfo.pDevice)
{
}

void ResourceLoader::loadImages(ImageLoadInfo& info)
{
    // if (std::holds_alternative<std::string>(info.data))
    // {
    //     std::filesystem::path path{std::get<std::string>(info.data)};
    // }

    // auto containerType = ImageContainerType::Default;

    // if(info.containerType == ImageContainerType::Default)
    // {
    //     if(path.extension() == ".ktx")
    //     {
    //         containerType = ImageContainerType::Ktx;
    //     }
    //     else if(path.extension() == ".png")
    //     {
    //         containerType = ImageContainerType::Png;
    //     }
    //     else if(path.extension() == ".jpg")
    //     {
    //         containerType = ImageContainerType::Jpg;
    //     }
    //     else
    //     {
    //         CM_LOG_ERR("Unsupported image format.");
    //         APH_ASSERT(false);
    //     }
    // }

    // vk::ImageCreateInfo ci;

    // if(info.pImageCI)
    // {
    //     ci = *info.pImageCI;
    // }

    // std::vector<uint8_t> data;

    // switch(containerType)
    // {
    // case ImageContainerType::Ktx:
    // {
    //     loadKTX(path, &ci, data);
    // }
    // break;
    // case ImageContainerType::Png:
    // case ImageContainerType::Jpg:
    // {
    //     loadPNGJPG(path, &ci, data);
    // }
    // break;
    // case ImageContainerType::Default:
    // case ImageContainerType::Dds:
    //     APH_ASSERT(false);
    // }

    // auto image = info.pImage;

    // m_pDevice->createDeviceLocalImage(ci, &image, data);
}

void ResourceLoader::loadBuffers(BufferLoadInfo& info)
{
    vk::BufferCreateInfo bufferCI = info.createInfo;

    // using staging buffer
    vk::Buffer* stagingBuffer{};
    {
        vk::BufferCreateInfo stagingCI{
            .size   = static_cast<uint32_t>(bufferCI.size),
            .usage  = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .domain = BufferDomain::Host,
        };

        m_pDevice->createBuffer(stagingCI, &stagingBuffer);

        m_pDevice->mapMemory(stagingBuffer);
        stagingBuffer->write(info.data);
        m_pDevice->unMapMemory(stagingBuffer);
    }

    {
        bufferCI.domain = BufferDomain::Device;
        bufferCI.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        m_pDevice->createBuffer(bufferCI, info.ppBuffer);
    }

    m_pDevice->executeSingleCommands(vk::QueueType::GRAPHICS, [&](vk::CommandBuffer* cmd) {
        cmd->copyBuffer(stagingBuffer, *info.ppBuffer, bufferCI.size);
    });

    m_pDevice->destroyBuffer(stagingBuffer);
}
}  // namespace aph
