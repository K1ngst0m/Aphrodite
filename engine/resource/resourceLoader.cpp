#include "resourceLoader.h"
#include "common/profiler.h"
#include "api/vulkan/device.h"
#include "tinyimageformat.h"

#include "shaderc/shaderc.hpp"

#include "slang.h"
#include "slang-com-ptr.h"

#define TINYKTX_IMPLEMENTATION
#include "tinyktx.h"

#define TINYDDS_IMPLEMENTATION
#include "tinydds.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <stb/stb_image.h>
#include <tinygltf/tiny_gltf.h>

#include "filesystem/filesystem.h"

namespace loader::image
{

inline std::shared_ptr<aph::ImageInfo> loadImageFromFile(std::string_view path, bool isFlipY = false)
{
    auto image = std::make_shared<aph::ImageInfo>();
    stbi_set_flip_vertically_on_load(isFlipY);
    int      width, height, channels;
    uint8_t* img = stbi_load(path.data(), &width, &height, &channels, 0);
    if(img == nullptr)
    {
        printf("Error in loading the image\n");
        exit(0);
    }
    // printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);
    image->width  = width;
    image->height = height;

    image->data.resize(width * height * 4);
    if(channels == 3)
    {
        std::vector<uint8_t> rgba(image->data.size());
        for(std::size_t i = 0; i < width * height; ++i)
        {
            memcpy(&rgba[4 * i], &img[3 * i], 3);
        }
        memcpy(image->data.data(), rgba.data(), image->data.size());
    }
    else
    {
        memcpy(image->data.data(), img, image->data.size());
    }
    stbi_image_free(img);

    return image;
}

inline std::array<std::shared_ptr<aph::ImageInfo>, 6> loadSkyboxFromFile(std::array<std::string_view, 6> paths)
{
    std::array<std::shared_ptr<aph::ImageInfo>, 6> skyboxImages;
    for(std::size_t idx = 0; idx < 6; idx++)
    {
        skyboxImages[idx] = loadImageFromFile(paths[idx]);
    }
    return skyboxImages;
}

inline bool loadKTX(const std::filesystem::path& path, aph::vk::ImageCreateInfo& outCI, std::vector<uint8_t>& data)
{
    if(!std::filesystem::exists(path))
    {
        CM_LOG_ERR("File does not exist: %s", path.c_str());
        return false;
    }

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if(!file.is_open())
    {
        CM_LOG_ERR("Failed to open file: %s", path.c_str());
        return false;
    }

    const auto ktxDataSize = static_cast<std::uint64_t>(file.tellg());
    if(ktxDataSize > UINT32_MAX)
    {
        CM_LOG_ERR("File too large: %s", path.c_str());
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

    aph::vk::ImageCreateInfo& textureCI = outCI;
    textureCI.extent                    = {
                           .width  = TinyKtx_Width(ctx),
                           .height = TinyKtx_Height(ctx),
                           .depth  = std::max(1U, TinyKtx_Depth(ctx)),
    };
    textureCI.arraySize = std::max(1U, TinyKtx_ArraySlices(ctx));
    textureCI.mipLevels = std::max(1U, TinyKtx_NumberOfMipmaps(ctx));
    textureCI.format    = aph::vk::utils::getFormatFromVk(
        (VkFormat)TinyImageFormat_ToVkFormat(TinyImageFormat_FromTinyKtxFormat(TinyKtx_GetFormat(ctx))));
    // textureCI.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    textureCI.sampleCount = 1;

    if(textureCI.format == aph::Format::Undefined)
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

inline bool loadDDS(const std::filesystem::path& path, aph::vk::ImageCreateInfo& outCI, std::vector<uint8_t>& data)
{
    if(!std::filesystem::exists(path))
    {
        CM_LOG_ERR("File does not exist: %s", path.c_str());
        return false;
    }

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if(!file.is_open())
    {
        CM_LOG_ERR("Failed to open file: %s", path.c_str());
        return false;
    }

    const auto ddsDataSize = static_cast<std::uint64_t>(file.tellg());
    if(ddsDataSize > UINT32_MAX)
    {
        CM_LOG_ERR("File too large: %s", path.c_str());
        return false;
    }

    file.seekg(0);

    TinyDDS_Callbacks callbacks{[](void* user, char const* msg) { CM_LOG_ERR("%s", msg); },
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

    TinyDDS_ContextHandle ctx        = TinyDDS_CreateContext(&callbacks, &file);
    bool                  headerOkay = TinyDDS_ReadHeader(ctx);
    if(!headerOkay)
    {
        TinyDDS_DestroyContext(ctx);
        return false;
    }

    aph::vk::ImageCreateInfo& textureCI = outCI;
    textureCI.extent                    = {
                           .width  = TinyDDS_Width(ctx),
                           .height = TinyDDS_Height(ctx),
                           .depth  = std::max(1U, TinyDDS_Depth(ctx)),
    };
    textureCI.arraySize = std::max(1U, TinyDDS_ArraySlices(ctx));
    textureCI.mipLevels = std::max(1U, TinyDDS_NumberOfMipmaps(ctx));
    textureCI.format    = aph::vk::utils::getFormatFromVk(
        (VkFormat)TinyImageFormat_ToVkFormat(TinyImageFormat_FromTinyDDSFormat(TinyDDS_GetFormat(ctx))));
    textureCI.sampleCount = 1;

    if(textureCI.format == aph::Format::Undefined)
    {
        TinyDDS_DestroyContext(ctx);
        return false;
    }

    if(TinyDDS_IsCubemap(ctx))
    {
        textureCI.arraySize *= 6;
    }

    TinyDDS_DestroyContext(ctx);

    return true;
}

inline bool loadPNGJPG(const std::filesystem::path& path, aph::vk::ImageCreateInfo& outCI, std::vector<uint8_t>& data)
{
    auto img = loadImageFromFile(path.c_str());

    if(img == nullptr)
    {
        return false;
    }

    aph::vk::ImageCreateInfo& textureCI = outCI;

    textureCI.extent = {
        .width  = img->width,
        .height = img->height,
        .depth  = 1,
    };

    textureCI.format = aph::Format::RGBA_UN8;

    data = img->data;

    return true;
}
}  // namespace loader::image

namespace loader::shader
{

std::vector<uint32_t> loadSpvFromFile(std::string_view filename)
{
    std::string source = aph::Filesystem::GetInstance().readFileToString(filename);
    APH_ASSERT(!source.empty());
    uint32_t              size = source.size();
    std::vector<uint32_t> spirv(size / sizeof(uint32_t));
    memcpy(spirv.data(), source.data(), size);
    return spirv;
}
std::vector<uint32_t> loadGlslFromFile(std::string_view filename)
{
    using namespace aph;
    shaderc::Compiler compiler{};
    std::string       source = aph::Filesystem::GetInstance().readFileToString(filename);
    APH_ASSERT(!source.empty());
    shaderc_shader_kind stage = shaderc_glsl_infer_from_source;
    switch(utils::getStageFromPath(filename))
    {
    case ShaderStage::VS:
        stage = shaderc_vertex_shader;
        break;
    case ShaderStage::FS:
        stage = shaderc_fragment_shader;
        break;
    case ShaderStage::CS:
        stage = shaderc_compute_shader;
        break;
    case ShaderStage::TCS:
        stage = shaderc_tess_control_shader;
        break;
    case ShaderStage::TES:
        stage = shaderc_tess_evaluation_shader;
        break;
    case ShaderStage::GS:
        stage = shaderc_geometry_shader;
        break;
    case ShaderStage::TS:
        stage = shaderc_task_shader;
        break;
    case ShaderStage::MS:
        stage = shaderc_mesh_shader;
        break;
    default:
        break;
    }

    shaderc::CompileOptions options{};
    options.SetGenerateDebugInfo();
    options.SetSourceLanguage(shaderc_source_language_glsl);
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    options.SetTargetSpirv(shaderc_spirv_version_1_6);

    auto result = compiler.CompileGlslToSpv(source.data(), source.size(), stage, filename.data(), "main", options);
    APH_ASSERT(result.GetCompilationStatus() == 0);
    std::vector<uint32_t> spirv{result.cbegin(), result.cend()};
    return spirv;
}

std::vector<uint32_t> loadSlangFromFile(std::string_view filename)
{
    using namespace slang;
    static Slang::ComPtr<IGlobalSession> globalSession;
    slang::createGlobalSession(globalSession.writeRef());

    {
        SessionDesc sessionDesc;

        TargetDesc targetDesc;
        targetDesc.format  = SLANG_SPIRV;
        targetDesc.profile = globalSession->findProfile("glsl_450");
        targetDesc.flags   = SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;

        sessionDesc.targets     = &targetDesc;
        sessionDesc.targetCount = 1;

        const char* searchPaths[]   = {"assets/shaders/slang"};
        sessionDesc.searchPaths     = searchPaths;
        sessionDesc.searchPathCount = 1;

        Slang::ComPtr<ISession> session;
        globalSession->createSession(sessionDesc, session.writeRef());

        // PreprocessorMacroDesc fancyFlag = { "ENABLE_FANCY_FEATURE", "1" };
        // sessionDesc.preprocessorMacros = &fancyFlag;
        // sessionDesc.preprocessorMacroCount = 1;

        Slang::ComPtr<IBlob> diagnostics;
        auto                 module = session->loadModule(filename.data(), diagnostics.writeRef());

        if(diagnostics)
        {
            CM_LOG_ERR("%s\n", (const char*)diagnostics->getBufferPointer());
            APH_ASSERT(false);
        }

        Slang::ComPtr<IEntryPoint> entryVS;
        module->findEntryPointByName("vertexMain", entryVS.writeRef());

        Slang::ComPtr<IEntryPoint> entryFS;
        module->findEntryPointByName("fragmentMain", entryFS.writeRef());

        IComponentType* components[] = {module, entryVS, entryFS};

        Slang::ComPtr<slang::IComponentType> linkedProgram;

        SlangResult result =
            session->createCompositeComponentType(components, 3, linkedProgram.writeRef(), diagnostics.writeRef());
        APH_ASSERT(SLANG_SUCCEEDED(result));

        Slang::ComPtr<slang::IBlob> spirvCode;
        {
            result = linkedProgram->getEntryPointCode(0, 0, spirvCode.writeRef(), diagnostics.writeRef());
            if(diagnostics)
            {
                CM_LOG_ERR("%s\n", (const char*)diagnostics->getBufferPointer());
                APH_ASSERT(false);
            }

            APH_ASSERT(SLANG_SUCCEEDED(result));
        }

        {
            std::vector<uint32_t> retSpvCode;
            retSpvCode.resize(spirvCode->getBufferSize() / 4);
            std::memcpy(retSpvCode.data(), spirvCode->getBufferPointer(), spirvCode->getBufferSize());
            return retSpvCode;
        }
    }
}
}  // namespace loader::shader

namespace loader::geometry
{
inline bool loadGLTF(aph::ResourceLoader* pLoader, const aph::GeometryLoadInfo& info, aph::Geometry** ppGeometry)
{
    auto path = std::filesystem::path{info.path};
    auto ext  = path.extension();

    bool               fileLoaded = false;
    tinygltf::Model    inputModel;
    tinygltf::TinyGLTF gltfContext;
    std::string        error, warning;

    if(ext == ".glb")
    {
        fileLoaded = gltfContext.LoadBinaryFromFile(&inputModel, &error, &warning, path);
    }
    else if(ext == ".gltf")
    {
        fileLoaded = gltfContext.LoadASCIIFromFile(&inputModel, &error, &warning, path);
    }

    if(!fileLoaded)
    {
        CM_LOG_ERR("%s", error);
        return false;
    }

    // TODO gltf loading
    *ppGeometry = new aph::Geometry;

    // Iterate over each mesh
    uint32_t vertexCount = 0;
    for(const auto& mesh : inputModel.meshes)
    {
        for(const auto& primitive : mesh.primitives)
        {
            // Index buffer
            const tinygltf::Accessor&   indexAccessor   = inputModel.accessors[primitive.indices];
            const tinygltf::BufferView& indexBufferView = inputModel.bufferViews[indexAccessor.bufferView];
            const tinygltf::Buffer&     indexBuffer     = inputModel.buffers[indexBufferView.buffer];

            {
                aph::vk::Buffer*    pIB;
                aph::BufferLoadInfo loadInfo{
                    .data = (void*)(indexBuffer.data.data() + indexBufferView.byteOffset),
                    // TODO index type
                    .createInfo = {.size  = static_cast<uint32_t>(indexAccessor.count * sizeof(uint16_t)),
                                   .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT}};
                pLoader->load(loadInfo, &pIB);
                (*ppGeometry)->indexBuffer.push_back(pIB);
            }

            // Vertex buffers
            for(const auto& attrib : primitive.attributes)
            {
                const tinygltf::Accessor&   accessor   = inputModel.accessors[attrib.second];
                const tinygltf::BufferView& bufferView = inputModel.bufferViews[accessor.bufferView];
                const tinygltf::Buffer&     buffer     = inputModel.buffers[bufferView.buffer];

                vertexCount += accessor.count;

                aph::vk::Buffer*    pVB;
                aph::BufferLoadInfo loadInfo{
                    .data       = (void*)(buffer.data.data() + bufferView.byteOffset),
                    .createInfo = {.size  = static_cast<uint32_t>(accessor.count * accessor.ByteStride(bufferView)),
                                   .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT},
                };
                pLoader->load(loadInfo, &pVB);
                (*ppGeometry)->vertexBuffers.push_back(pVB);
                (*ppGeometry)->vertexStrides.push_back(accessor.ByteStride(bufferView));
            }

            // TODO: Load draw arguments, handle materials, optimize geometry etc.

        }  // End of iterating through primitives
    }      // End of iterating through meshes

    const uint32_t indexStride = vertexCount > UINT16_MAX ? sizeof(uint32_t) : sizeof(uint16_t);
    (*ppGeometry)->indexType   = (sizeof(uint32_t) == indexStride) ? aph::IndexType::UINT16 : aph::IndexType::UINT32;

    return true;
}

}  // namespace loader::geometry

namespace aph
{

ImageContainerType GetImageContainerType(const std::filesystem::path& path)
{
    if(path.extension() == ".ktx")
    {
        return ImageContainerType::Ktx;
    }

    if(path.extension() == ".png")
    {
        return ImageContainerType::Png;
    }

    if(path.extension() == ".jpg")
    {
        return ImageContainerType::Jpg;
    }

    CM_LOG_ERR("Unsupported image format.");

    return ImageContainerType::Default;
}

ResourceLoader::ResourceLoader(const ResourceLoaderCreateInfo& createInfo) :
    m_createInfo(createInfo),
    m_pDevice(createInfo.pDevice)
{
    m_pQueue = m_pDevice->getQueue(QueueType::Transfer);
}

ResourceLoader::~ResourceLoader() = default;

void ResourceLoader::load(const ImageLoadInfo& info, vk::Image** ppImage)
{
    std::filesystem::path path;
    std::vector<uint8_t>  data;
    vk::ImageCreateInfo   ci;
    if(info.pCreateInfo)
    {
        ci = *info.pCreateInfo;
    }

    if(std::holds_alternative<std::string>(info.data))
    {
        path = aph::Filesystem::GetInstance().resolvePath(std::get<std::string>(info.data));

        auto containerType =
            info.containerType == ImageContainerType::Default ? GetImageContainerType(path) : info.containerType;

        switch(containerType)
        {
        case ImageContainerType::Ktx:
        {
            loader::image::loadKTX(path, ci, data);
        }
        break;
        case ImageContainerType::Png:
        case ImageContainerType::Jpg:
        {
            loader::image::loadPNGJPG(path, ci, data);
        }
        break;
        case ImageContainerType::Dds:
        {
            loader::image::loadDDS(path, ci, data);
        }
        break;
        case ImageContainerType::Default:
            APH_ASSERT(false);
            return;
        }
    }
    else if(std::holds_alternative<ImageInfo>(info.data))
    {
        auto img  = std::get<ImageInfo>(info.data);
        data      = img.data;
        ci.extent = {img.width, img.height, 1};
    }

    bool           genMipmap = ci.mipLevels > 1;
    const uint32_t width     = ci.extent.width;
    const uint32_t height    = ci.extent.height;

    // Load texture from image buffer
    vk::Buffer* stagingBuffer;
    {
        vk::BufferCreateInfo bufferCI{
            .size   = static_cast<uint32_t>(data.size()),
            .usage  = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .domain = BufferDomain::Host,
        };
        APH_CHECK_RESULT(
            m_pDevice->create(bufferCI, &stagingBuffer, std::string{info.debugName} + std::string{"_staging"}));

        APH_CHECK_RESULT(m_pDevice->mapMemory(stagingBuffer));
        writeBuffer(stagingBuffer, data.data());
        m_pDevice->unMapMemory(stagingBuffer);
    }

    vk::Image* image{};
    {
        auto imageCI = ci;
        imageCI.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageCI.domain = ImageDomain::Device;
        if(genMipmap)
        {
            imageCI.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }

        APH_CHECK_RESULT(m_pDevice->create(imageCI, &image, info.debugName));

        auto queue = m_pQueue;
        m_pDevice->executeSingleCommands(queue, [&](vk::CommandBuffer* cmd) {
            cmd->transitionImageLayout(image, aph::RESOURCE_STATE_COPY_DST);

            cmd->copyBufferToImage(stagingBuffer, image);
            if(genMipmap)
            {
                cmd->transitionImageLayout(image, aph::RESOURCE_STATE_COPY_SRC);
            }
        });

        // TODO mip map opeartions
        // executeSingleCommands(queue, [&](vk::CommandBuffer* cmd) {
        //     if(genMipmap)
        //     {
        //         // generate mipmap chains
        //         for(int32_t i = 1; i < imageCI.mipLevels; i++)
        //         {
        //             VkImageBlit imageBlit{};

        //             // Source
        //             imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        //             imageBlit.srcSubresource.layerCount = 1;
        //             imageBlit.srcSubresource.mipLevel   = i - 1;
        //             imageBlit.srcOffsets[1].x           = int32_t(width >> (i - 1));
        //             imageBlit.srcOffsets[1].y           = int32_t(height >> (i - 1));
        //             imageBlit.srcOffsets[1].z           = 1;

        //             // Destination
        //             imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        //             imageBlit.dstSubresource.layerCount = 1;
        //             imageBlit.dstSubresource.mipLevel   = i;
        //             imageBlit.dstOffsets[1].x           = int32_t(width >> i);
        //             imageBlit.dstOffsets[1].y           = int32_t(height >> i);
        //             imageBlit.dstOffsets[1].z           = 1;

        //             // Prepare current mip level as image blit destination
        //             vk::ImageBarrier barrier{
        //                 .pImage             = image,
        //                 .currentState       = image->getResourceState(),
        //                 .newState           = RESOURCE_STATE_COPY_DST,
        //                 .subresourceBarrier = 1,
        //                 .mipLevel           = static_cast<uint8_t>(imageCI.mipLevels),
        //             };

        //             cmd->insertBarrier({barrier});
        //             // Blit from previous level
        //             cmd->blitImage(image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
        //                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);
        //             barrier.currentState = image->getResourceState();
        //             barrier.newState     = RESOURCE_STATE_COPY_SRC;
        //             cmd->insertBarrier({barrier});
        //         }
        //     }

        //     cmd->transitionImageLayout(image, RESOURCE_STATE_SHADER_RESOURCE);
        // });
    }

    m_pDevice->destroy(stagingBuffer);
    *ppImage = image;
}

void ResourceLoader::load(const BufferLoadInfo& info, vk::Buffer** ppBuffer)
{
    vk::BufferCreateInfo bufferCI = info.createInfo;

    {
        bufferCI.domain = bufferCI.domain;
        bufferCI.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        APH_CHECK_RESULT(m_pDevice->create(bufferCI, ppBuffer, info.debugName));
    }

    // update buffer
    if(info.data)
    {
        this->update(
            BufferUpdateInfo{
                .data  = info.data,
                .range = {0, info.createInfo.size},
            },
            ppBuffer);
    }
}

void ResourceLoader::load(const ShaderLoadInfo& info, vk::Shader** ppShader)
{
    auto uuid = m_uuidGenerator.getUUID().str();

    VkShaderModuleCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    };
    std::vector<uint32_t> spvCode;
    if(std::holds_alternative<std::string>(info.data))
    {
        std::filesystem::path path = std::get<std::string>(info.data);

        // TODO override with new load info
        if(m_shaderUUIDMap.contains(path))
        {
            *ppShader = m_shaderModuleCaches.at(m_shaderUUIDMap.at(path)).get();
            return;
        }

        if(path.extension() == ".spv")
        {
            spvCode = loader::shader::loadSpvFromFile(path.string());
        }
        else if(path.extension() == ".vert" || path.extension() == ".frag" || path.extension() == ".comp" ||
                path.extension() == ".geom")
        {
            spvCode = loader::shader::loadGlslFromFile(path.string());
        }
        else if(path.extension() == ".slang")
        {
            spvCode = loader::shader::loadSlangFromFile(path.string());
        }
        else
        {
            CM_LOG_ERR("Unsupported shader format: %s", path.extension().string());
            APH_ASSERT(false);
        }
        APH_ASSERT(!spvCode.empty());

        createInfo.codeSize = spvCode.size() * sizeof(spvCode[0]);
        createInfo.pCode    = spvCode.data();

        m_shaderUUIDMap[path] = uuid;
    }
    else if(std::holds_alternative<std::vector<uint32_t>>(info.data))
    {
        auto& code          = std::get<std::vector<uint32_t>>(info.data);
        createInfo.codeSize = code.size() * sizeof(code[0]);
        createInfo.pCode    = code.data();

        {
            spvCode = code;
        }
    }

    // TODO macro
    {
    }

    VkShaderModule handle;
    _VR(m_pDevice->getDeviceTable()->vkCreateShaderModule(m_pDevice->getHandle(), &createInfo, vk::vkAllocator(),
                                                          &handle));

    APH_ASSERT(!m_shaderModuleCaches.contains(uuid));
    m_shaderModuleCaches[uuid] = std::make_unique<vk::Shader>(spvCode, handle, info.entryPoint);

    *ppShader = m_shaderModuleCaches[uuid].get();
}

void ResourceLoader::cleanup()
{
    for(const auto& [_, shaderModule] : m_shaderModuleCaches)
    {
        m_pDevice->getDeviceTable()->vkDestroyShaderModule(m_pDevice->getHandle(), shaderModule->getHandle(),
                                                           vk::vkAllocator());
    }
}

void ResourceLoader::load(const GeometryLoadInfo& info, Geometry** ppGeometry)
{
    auto path = std::filesystem::path{info.path};
    auto ext  = path.extension();

    if(ext == ".glb" || ext == ".gltf")
    {
        loader::geometry::loadGLTF(this, info, ppGeometry);
    }
    else
    {
        CM_LOG_ERR("Unsupported model file type: %s.", ext);
        APH_ASSERT(false);
    }
}

void ResourceLoader::update(const BufferUpdateInfo& info, vk::Buffer** ppBuffer)
{
    auto pBuffer    = *ppBuffer;
    auto domain     = pBuffer->getCreateInfo().domain;
    auto uploadSize = info.range.size;

    // device only
    if(domain == BufferDomain::Device || domain == BufferDomain::LinkedDeviceHostPreferDevice)
    {
        if(info.range.size == VK_WHOLE_SIZE)
        {
            uploadSize = pBuffer->getSize();
        }

        if(uploadSize <= LIMIT_BUFFER_CMD_UPDATE_SIZE)
        {
            PROFILE_SCOPE("loading data by: vkCmdBufferUpdate.");
            m_pDevice->executeSingleCommands(m_pQueue, [=](vk::CommandBuffer* cmd) {
                cmd->updateBuffer(*ppBuffer, {0, uploadSize}, info.data);
            });
        }
        else
        {
            PROFILE_SCOPE("loading data by: staging copy.");
            for(std::size_t offset = info.range.offset; offset < uploadSize; offset += LIMIT_BUFFER_UPLOAD_SIZE)
            {
                MemoryRange copyRange = {
                    .offset = offset,
                    .size   = std::min(std::size_t{LIMIT_BUFFER_UPLOAD_SIZE}, {uploadSize - offset}),
                };

                // using staging buffer
                vk::Buffer* stagingBuffer{};
                {
                    vk::BufferCreateInfo stagingCI{
                        .size   = static_cast<uint32_t>(copyRange.size),
                        .usage  = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        .domain = BufferDomain::Host,
                    };

                    APH_CHECK_RESULT(m_pDevice->create(stagingCI, &stagingBuffer,
                                                       std::string{info.debugName} + std::string{"_staging"}));

                    APH_CHECK_RESULT(m_pDevice->mapMemory(stagingBuffer));
                    writeBuffer(stagingBuffer, info.data, {0, copyRange.size});
                    m_pDevice->unMapMemory(stagingBuffer);
                }

                m_pDevice->executeSingleCommands(
                    m_pQueue, [=](vk::CommandBuffer* cmd) { cmd->copyBuffer(stagingBuffer, *ppBuffer, copyRange); });

                m_pDevice->destroy(stagingBuffer);
            }
        }
    }
    else
    {
        PROFILE_SCOPE("loading data by: vkMapMemory.");
        writeBuffer(pBuffer, info.data, info.range);
    }
}

void ResourceLoader::writeBuffer(vk::Buffer* pBuffer, const void* data, MemoryRange range)
{
    auto domain = pBuffer->getCreateInfo().domain;
    APH_ASSERT(domain != BufferDomain::Device && domain != BufferDomain::LinkedDeviceHostPreferDevice);
    APH_ASSERT(pBuffer->getMapped());
    if(range.size == 0)
    {
        range.size = VK_WHOLE_SIZE;
    }

    if(range.size == VK_WHOLE_SIZE || range.size == 0)
    {
        range.size = pBuffer->getSize();
    }

    uint8_t* pMapped = (uint8_t*)pBuffer->getMapped();
    memcpy(pMapped + range.offset, data, range.size);
}
}  // namespace aph
