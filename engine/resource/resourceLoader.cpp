#include "resourceLoader.h"
#include "common/common.h"
#include "common/profiler.h"
#include "api/vulkan/device.h"

#include "slang.h"
#include "slang-com-ptr.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "tiny_gltf.h"

#include "filesystem/filesystem.h"

namespace loader::image
{

inline std::shared_ptr<aph::ImageInfo> loadImageFromFile(std::string_view path, bool isFlipY = false)
{
    APH_PROFILER_SCOPE();
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
    APH_PROFILER_SCOPE();
    std::array<std::shared_ptr<aph::ImageInfo>, 6> skyboxImages;
    for(std::size_t idx = 0; idx < 6; idx++)
    {
        skyboxImages[idx] = loadImageFromFile(paths[idx]);
    }
    return skyboxImages;
}

inline bool loadKTX(const std::filesystem::path& path, aph::vk::ImageCreateInfo& outCI, std::vector<uint8_t>& data)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(false);
    return false;
}

inline bool loadPNGJPG(const std::filesystem::path& path, aph::vk::ImageCreateInfo& outCI, std::vector<uint8_t>& data)
{
    APH_PROFILER_SCOPE();
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

    textureCI.format = aph::Format::RGBA8_UNORM;

    data = img->data;

    return true;
}
}  // namespace loader::image

namespace loader::shader
{

std::vector<uint32_t> loadSpvFromFile(std::string_view filename)
{
    APH_PROFILER_SCOPE();
    std::string source = aph::Filesystem::GetInstance().readFileToString(filename);
    APH_ASSERT(!source.empty());
    uint32_t              size = source.size();
    std::vector<uint32_t> spirv(size / sizeof(uint32_t));
    memcpy(spirv.data(), source.data(), size);
    return spirv;
}

aph::HashMap<aph::ShaderStage, std::vector<uint32_t>> loadSlangFromFile(std::string_view filename)
{
    APH_PROFILER_SCOPE();
    using namespace slang;
    static Slang::ComPtr<IGlobalSession> globalSession;
    slang::createGlobalSession(globalSession.writeRef());

    SessionDesc sessionDesc;

    TargetDesc targetDesc;
    targetDesc.format  = SLANG_SPIRV;
    targetDesc.profile = globalSession->findProfile("spirv");
    targetDesc.flags   = SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;

    sessionDesc.targets     = &targetDesc;
    sessionDesc.targetCount = 1;

    // TODO protocol
    const char* searchPaths[]   = {"assets/shaders/slang"};
    sessionDesc.searchPaths     = searchPaths;
    sessionDesc.searchPathCount = 1;

    Slang::ComPtr<ISession> session;
    globalSession->createSession(sessionDesc, session.writeRef());

    // PreprocessorMacroDesc fancyFlag = { "ENABLE_FANCY_FEATURE", "1" };
    // sessionDesc.preprocessorMacros = &fancyFlag;
    // sessionDesc.preprocessorMacroCount = 1;

    Slang::ComPtr<IBlob> diagnostics;

    auto fname = aph::Filesystem::GetInstance().resolvePath(filename);
    auto                 module = session->loadModule(fname.c_str(), diagnostics.writeRef());

    if(diagnostics)
    {
        auto errlog = (const char*)diagnostics->getBufferPointer();
        CM_LOG_ERR("[slang diagnostics]: %s", errlog);
        APH_ASSERT(false);
        return {};
    }

    struct EntryPointData
    {
        Slang::ComPtr<IEntryPoint> entryPoint{};
        SlangResult result{};
    };

    aph::HashMap<aph::ShaderStage, std::vector<uint32_t>> spvCodes;
    std::vector<EntryPointData> entryPoints;

    struct StageEntry{
        std::string_view entryPoint;
        aph::ShaderStage aphStage;
        SlangStage slangStage;
    };
    const aph::SmallVector<StageEntry> entryPointCandidates =
    {
        {"vertexMain", aph::ShaderStage::VS, SLANG_STAGE_VERTEX},
        {"fragmentMain", aph::ShaderStage::FS, SLANG_STAGE_FRAGMENT},
        {"computeMain", aph::ShaderStage::CS, SLANG_STAGE_COMPUTE},
        {"taskMain", aph::ShaderStage::TS, SLANG_STAGE_AMPLIFICATION},
        {"meshMain", aph::ShaderStage::MS, SLANG_STAGE_MESH},
    };

    for (const auto& epCandidate: entryPointCandidates)
    {
        EntryPointData entryPointData{};
        auto& findEntryPointResult = entryPointData.result;
        auto& entryPoint = entryPointData.entryPoint;
        findEntryPointResult = module->findAndCheckEntryPoint(epCandidate.entryPoint.data(), epCandidate.slangStage, entryPoint.writeRef(), diagnostics.writeRef());

        if(!entryPoint)
        {
            continue;
        }

        // TODO
        if(diagnostics || !SLANG_SUCCEEDED(findEntryPointResult))
        {
            auto errlog = (const char*)diagnostics->getBufferPointer();
            CM_LOG_ERR("[slang diagnostics]: %s", errlog);
            APH_ASSERT(false);
            return {};
        }

        IComponentType* components[] = {module, entryPoint};

        Slang::ComPtr<slang::IComponentType> linkedProgram;

        SlangResult result =
            session->createCompositeComponentType(components, 2, linkedProgram.writeRef(), diagnostics.writeRef());
        if(!SLANG_SUCCEEDED(result))
        {
            APH_ASSERT(false);
            return {};
        }

        Slang::ComPtr<slang::IBlob> spirvCode;
        {
            result = linkedProgram->getEntryPointCode(0, 0, spirvCode.writeRef(), diagnostics.writeRef());
            if(diagnostics)
            {
                CM_LOG_ERR("%s\n", (const char*)diagnostics->getBufferPointer());
                APH_ASSERT(false);
                return {};
            }

            APH_ASSERT(SLANG_SUCCEEDED(result));
        }

        {
            std::vector<uint32_t> retSpvCode;
            retSpvCode.resize(spirvCode->getBufferSize() / 4);
            std::memcpy(retSpvCode.data(), spirvCode->getBufferPointer(), spirvCode->getBufferSize());
            spvCodes[epCandidate.aphStage] = std::move(retSpvCode);
        }
    }

    // for (const auto& stage: stages)
    // {
    //     APH_ASSERT(!spvCodes.contains(stage));
    //     EntryPointData entryPointData{};
    //     auto& findEntryPointResult = entryPointData.result;
    //     auto& entryPoint = entryPointData.entryPoint;
    //     switch(stage)
    //     {
    //     case aph::ShaderStage::VS:
    //         findEntryPointResult = module->findAndCheckEntryPoint("vertexMain", SLANG_STAGE_VERTEX,
    //         entryPoint.writeRef(), diagnostics.writeRef()); break;
    //     case aph::ShaderStage::FS:
    //         findEntryPointResult = module->findAndCheckEntryPoint("fragmentMain", SLANG_STAGE_FRAGMENT,
    //         entryPoint.writeRef(), diagnostics.writeRef()); break;
    //     case aph::ShaderStage::CS:
    //         findEntryPointResult = module->findAndCheckEntryPoint("computeMain", SLANG_STAGE_COMPUTE,
    //         entryPoint.writeRef(), diagnostics.writeRef()); break;
    //     case aph::ShaderStage::TS:
    //         findEntryPointResult = module->findAndCheckEntryPoint("taskMain", SLANG_STAGE_AMPLIFICATION,
    //         entryPoint.writeRef(), diagnostics.writeRef()); break;
    //     case aph::ShaderStage::MS:
    //         findEntryPointResult = module->findAndCheckEntryPoint("meshMain", SLANG_STAGE_MESH,
    //         entryPoint.writeRef(), diagnostics.writeRef()); break;
    //     default:
    //         APH_ASSERT(false);
    //         return {};
    //     }

    //     if (diagnostics || findEntryPointResult)
    //     {
    //         auto errlog = (const char*)diagnostics->getBufferPointer();
    //         CM_LOG_ERR("[slang diagnostics]: %s", errlog);
    //         APH_ASSERT(false);
    //         return {};
    //     }

    //     // entryPoints.push_back(std::move(entryPointData));
    //     IComponentType* components[] = {module, entryPoint};

    //     Slang::ComPtr<slang::IComponentType> linkedProgram;

    //     SlangResult result =
    //         session->createCompositeComponentType(components, 2, linkedProgram.writeRef(), diagnostics.writeRef());
    //     if (!SLANG_SUCCEEDED(result))
    //     {
    //         APH_ASSERT(false);
    //         return {};
    //     }

    //     Slang::ComPtr<slang::IBlob> spirvCode;
    //     {
    //         result = linkedProgram->getEntryPointCode(0, 0, spirvCode.writeRef(), diagnostics.writeRef());
    //         if(diagnostics)
    //         {
    //             CM_LOG_ERR("%s\n", (const char*)diagnostics->getBufferPointer());
    //             APH_ASSERT(false);
    //             return {};
    //         }

    //         APH_ASSERT(SLANG_SUCCEEDED(result));
    //     }

    //     {
    //         std::vector<uint32_t> retSpvCode;
    //         retSpvCode.resize(spirvCode->getBufferSize() / 4);
    //         std::memcpy(retSpvCode.data(), spirvCode->getBufferPointer(), spirvCode->getBufferSize());
    //         spvCodes[stage] = std::move(retSpvCode);
    //     }
    // }
    return spvCodes;
}

}  // namespace loader::shader

namespace loader::geometry
{
inline bool loadGLTF(aph::ResourceLoader* pLoader, const aph::GeometryLoadInfo& info, aph::Geometry** ppGeometry)
{
    APH_PROFILER_SCOPE();
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
                APH_VR(pLoader->load(loadInfo, &pIB));
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
                APH_VR(pLoader->load(loadInfo, &pVB));
                (*ppGeometry)->vertexBuffers.push_back(pVB);
                (*ppGeometry)->vertexStrides.push_back(accessor.ByteStride(bufferView));
            }

            // TODO: Load draw arguments, handle materials, optimize geometry etc.

        }  // End of iterating through primitives
    }  // End of iterating through meshes

    const uint32_t indexStride = vertexCount > UINT16_MAX ? sizeof(uint32_t) : sizeof(uint16_t);
    (*ppGeometry)->indexType   = (sizeof(uint32_t) == indexStride) ? aph::IndexType::UINT16 : aph::IndexType::UINT32;

    return true;
}

}  // namespace loader::geometry

namespace aph
{

ImageContainerType GetImageContainerType(const std::filesystem::path& path)
{
    APH_PROFILER_SCOPE();
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

Result ResourceLoader::load(const ImageLoadInfo& info, vk::Image** ppImage)
{
    APH_PROFILER_SCOPE();
    std::filesystem::path path;
    std::vector<uint8_t>  data;
    vk::ImageCreateInfo   ci;
    ci = info.createInfo;

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
        case ImageContainerType::Default:
            APH_ASSERT(false);
            return {Result::RuntimeError, "Unsupported image type."};
        }
    }
    else if(std::holds_alternative<ImageInfo>(info.data))
    {
        auto img  = std::get<ImageInfo>(info.data);
        data      = img.data;
        ci.extent = {img.width, img.height, 1};
    }

    // Load texture from image buffer
    vk::Buffer* stagingBuffer;
    {
        vk::BufferCreateInfo bufferCI{
            .size   = static_cast<uint32_t>(data.size()),
            .usage  = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .domain = BufferDomain::Host,
        };
        APH_VR(m_pDevice->create(bufferCI, &stagingBuffer, std::string{info.debugName} + std::string{"_staging"}));

        writeBuffer(stagingBuffer, data.data());
    }

    vk::Image* image{};

    {
        bool genMipmap = ci.mipLevels > 1;

        auto imageCI = ci;
        imageCI.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageCI.domain = ImageDomain::Device;
        if(genMipmap)
        {
            imageCI.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }

        APH_VR(m_pDevice->create(imageCI, &image, info.debugName));

        auto queue = m_pQueue;

        // mip map opeartions
        m_pDevice->executeSingleCommands(queue, [&](auto* cmd) {
            cmd->transitionImageLayout(image, aph::ResourceState::CopyDest);
            cmd->copyBufferToImage(stagingBuffer, image);

            if(genMipmap)
            {
                cmd->transitionImageLayout(image, aph::ResourceState::CopySource);
                int32_t width  = ci.extent.width;
                int32_t height = ci.extent.height;

                // generate mipmap chains
                for(int32_t i = 1; i < imageCI.mipLevels; i++)
                {
                    vk::ImageBlitInfo srcBlitInfo{
                        .extent     = {int32_t(width >> (i - 1)), int32_t(height >> (i - 1)), 1},
                        .level      = static_cast<uint32_t>(i - 1),
                        .layerCount = 1,
                    };

                    vk::ImageBlitInfo dstBlitInfo{
                        .extent     = {int32_t(width >> i), int32_t(height >> i), 1},
                        .level      = static_cast<uint32_t>(i),
                        .layerCount = 1,
                    };

                    // Prepare current mip level as image blit destination
                    vk::ImageBarrier barrier{
                        .pImage             = image,
                        .currentState       = image->getResourceState(),
                        .newState           = ResourceState::CopyDest,
                        .subresourceBarrier = 1,
                        .mipLevel           = static_cast<uint8_t>(imageCI.mipLevels),
                    };
                    cmd->insertBarrier({barrier});

                    // Blit from previous level
                    cmd->blitImage(image, image, srcBlitInfo, dstBlitInfo);

                    barrier.currentState = image->getResourceState();
                    barrier.newState     = ResourceState::CopySource;
                    cmd->insertBarrier({barrier});
                }
            }

            cmd->transitionImageLayout(image, ResourceState::ShaderResource);
        });
    }

    m_pDevice->destroy(stagingBuffer);
    *ppImage = image;

    return Result::Success;
}

Result ResourceLoader::load(const BufferLoadInfo& info, vk::Buffer** ppBuffer)
{
    APH_PROFILER_SCOPE();
    vk::BufferCreateInfo bufferCI = info.createInfo;

    {
        bufferCI.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        APH_VR(m_pDevice->create(bufferCI, ppBuffer, info.debugName));
    }

    // update buffer
    if(info.data)
    {
        this->update(
            {
                .data  = info.data,
                .range = {0, info.createInfo.size},
            },
            ppBuffer);
    }

    return Result::Success;
}

Result ResourceLoader::load(const ShaderLoadInfo& info, vk::ShaderProgram** ppProgram)
{
    APH_PROFILER_SCOPE();

    HashMap<ShaderStage, vk::Shader*> requiredShaderList;
    HashMap<std::filesystem::path, HashSet<aph::ShaderStage>> requiredStageMaps;
    for(auto& [stage, stageLoadInfo] : info.stageInfo)
    {
        if (std::holds_alternative<std::string>(stageLoadInfo.data))
        {
            auto path = Filesystem::GetInstance().resolvePath(std::get<std::string>(stageLoadInfo.data));
            requiredStageMaps[path].insert(stage);
        }
        else
        {
            requiredShaderList[stage] = loadShader(std::get<std::vector<uint32_t>>(stageLoadInfo.data));
        }
    }

    for(const auto& [path, requiredStages] : requiredStageMaps)
    {
        if (m_shaderCaches.contains(path.string()))
        {
            const auto& shaderCache = m_shaderCaches[path.string()];
            for (const auto& stage: requiredStages)
            {
                APH_ASSERT(shaderCache.contains(stage));
                requiredShaderList[stage] = shaderCache.at(stage);
            }
            continue;
        }

        if(path.extension() == ".spv")
        {
            auto shader = loadShader(loader::shader::loadSpvFromFile(path.c_str()));
            // TODO multi shader stage single spv binary support
            auto stage = *requiredStages.cbegin();
            requiredShaderList[stage] = shader;
            m_shaderCaches[path][stage] = shader;
        }
        else if(path.extension() == ".slang")
        {
            auto spvCodeMap = loader::shader::loadSlangFromFile(path.c_str());
            for (const auto& [stage, spv]: spvCodeMap)
            {
                APH_ASSERT(!requiredShaderList.contains(stage));

                auto shader       = loadShader(spv);
                m_shaderCaches[path][stage] = shader;

                if (requiredStages.contains(stage))
                {
                    requiredShaderList[stage] = shader;
                }
            }
        }
        else
        {
            CM_LOG_ERR("Unsupported shader format: %s", path.extension().string());
            APH_ASSERT(false);
            return {Result::RuntimeError, "Unsupported shader format."};
        }
    }

    // vs + fs
    if(requiredShaderList.contains(ShaderStage::VS) && requiredShaderList.contains(ShaderStage::FS))
    {
        APH_VR(m_pDevice->create(
            vk::ProgramCreateInfo{
                .geometry{.pVertex = requiredShaderList[ShaderStage::VS], .pFragment = requiredShaderList[ShaderStage::FS]},
                .type = PipelineType::Geometry,
            },
            ppProgram));
    }
    else if(requiredShaderList.contains(ShaderStage::MS) && requiredShaderList.contains(ShaderStage::FS))
    {
        vk::ProgramCreateInfo ci{
            .mesh{.pMesh = requiredShaderList[ShaderStage::MS], .pFragment = requiredShaderList[ShaderStage::FS]},
            .type = PipelineType::Mesh,
        };
        if(requiredShaderList.contains(ShaderStage::TS))
        {
            ci.mesh.pTask = requiredShaderList[ShaderStage::TS];
        }
        APH_VR(m_pDevice->create(ci, ppProgram));
    }
    // cs
    else if(requiredShaderList.contains(ShaderStage::CS))
    {
        APH_VR(m_pDevice->create(
            vk::ProgramCreateInfo{
                .compute{.pCompute = requiredShaderList[ShaderStage::CS]},
                .type = PipelineType::Compute,
            },
            ppProgram));
    }
    else
    {
        APH_ASSERT(false);
        return { Result::RuntimeError , "Unsupported shader stage combinations."};
    }

    return Result::Success;
}

void ResourceLoader::cleanup()
{
    APH_PROFILER_SCOPE();
    for(const auto& [_, shaderCache] : m_shaderCaches)
    {
        for(const auto& [_, shader] : shaderCache)
        {
            m_pDevice->getDeviceTable()->vkDestroyShaderModule(m_pDevice->getHandle(),
                                                            shader->getHandle(),
                                                            vk::vkAllocator());
        }
    }
}

Result ResourceLoader::load(const GeometryLoadInfo& info, Geometry** ppGeometry)
{
    APH_PROFILER_SCOPE();
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
        return {Result::RuntimeError, "Unsupported model file type."};
    }
    return Result::Success;
}

void ResourceLoader::update(const BufferUpdateInfo& info, vk::Buffer** ppBuffer)
{
    APH_PROFILER_SCOPE();
    auto pBuffer    = *ppBuffer;
    auto domain     = pBuffer->getCreateInfo().domain;
    auto uploadSize = info.range.size;

    // device only
    if(domain == BufferDomain::Device)
    {
        if(info.range.size == VK_WHOLE_SIZE)
        {
            uploadSize = pBuffer->getSize();
        }

        if(uploadSize <= LIMIT_BUFFER_CMD_UPDATE_SIZE)
        {
            APH_PROFILER_SCOPE_NAME("loading data by: vkCmdBufferUpdate.");
            m_pDevice->executeSingleCommands(m_pQueue, [=](auto* cmd) {
                cmd->updateBuffer(pBuffer, {0, uploadSize}, info.data);
            });
        }
        else
        {
            APH_PROFILER_SCOPE_NAME("loading data by: staging copy.");
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

                    APH_VR(m_pDevice->create(stagingCI, &stagingBuffer,
                                             std::string{info.debugName} + std::string{"_staging"}));

                    writeBuffer(stagingBuffer, info.data, {0, copyRange.size});
                }

                m_pDevice->executeSingleCommands(
                    m_pQueue, [=](auto* cmd) { cmd->copyBuffer(stagingBuffer, pBuffer, copyRange); });

                m_pDevice->destroy(stagingBuffer);
            }
        }
    }
    else
    {
        APH_PROFILER_SCOPE_NAME("loading data by: vkMapMemory.");
        writeBuffer(pBuffer, info.data, info.range);
    }
}

void ResourceLoader::writeBuffer(vk::Buffer* pBuffer, const void* data, MemoryRange range)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(pBuffer->getCreateInfo().domain != BufferDomain::Device);
    if(range.size == 0)
    {
        range.size = VK_WHOLE_SIZE;
    }

    if(range.size == VK_WHOLE_SIZE || range.size == 0)
    {
        range.size = pBuffer->getSize();
    }

    void* pMapped = {};
    APH_VR(m_pDevice->mapMemory(pBuffer, &pMapped));
    std::memcpy((uint8_t*)pMapped + range.offset, data, range.size);
    m_pDevice->unMapMemory(pBuffer);
}

vk::Shader* ResourceLoader::loadShader(const std::vector<uint32_t>& spv)
{
    VkShaderModuleCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spv.size() * sizeof(spv[0]),
        .pCode = spv.data(),
    };
    VkShaderModule handle;
    _VR(m_pDevice->getDeviceTable()->vkCreateShaderModule(m_pDevice->getHandle(), &createInfo, vk::vkAllocator(),
                                                          &handle));
    // TODO entry point input
    return m_shaderPool.allocate(spv, handle, "main");
}
}  // namespace aph
