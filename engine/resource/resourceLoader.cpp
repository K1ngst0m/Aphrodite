#include "resourceLoader.h"
#include "api/vulkan/device.h"
#include "common/common.h"
#include "common/profiler.h"

#include "slang-com-ptr.h"
#include "slang.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "tiny_gltf.h"

#include "filesystem/filesystem.h"

namespace aph::loader::image
{
inline std::shared_ptr<aph::ImageInfo> loadImageFromFile(std::string_view path, bool isFlipY = false)
{
    APH_PROFILER_SCOPE();
    auto image = std::make_shared<aph::ImageInfo>();
    stbi_set_flip_vertically_on_load(isFlipY);
    int width, height, channels;
    uint8_t* img = stbi_load(path.data(), &width, &height, &channels, 0);
    if (img == nullptr)
    {
        printf("Error in loading the image\n");
        exit(0);
    }
    // printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);
    image->width = width;
    image->height = height;

    image->data.resize(width * height * 4);
    if (channels == 3)
    {
        std::vector<uint8_t> rgba(image->data.size());
        for (std::size_t i = 0; i < width * height; ++i)
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
    for (std::size_t idx = 0; idx < 6; idx++)
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

    if (img == nullptr)
    {
        return false;
    }

    aph::vk::ImageCreateInfo& textureCI = outCI;

    textureCI.extent = {
        .width = img->width,
        .height = img->height,
        .depth = 1,
    };

    textureCI.format = aph::Format::RGBA8_UNORM;

    data = img->data;

    return true;
}
} // namespace aph::loader::image

namespace aph::loader::shader
{

std::vector<uint32_t> loadSpvFromFile(std::string_view filename)
{
    APH_PROFILER_SCOPE();
    std::string source = aph::Filesystem::GetInstance().readFileToString(filename);
    APH_ASSERT(!source.empty());
    uint32_t size = source.size();
    std::vector<uint32_t> spirv(size / sizeof(uint32_t));
    memcpy(spirv.data(), source.data(), size);
    return spirv;
}

#define SLANG_CR(diagnostics)                                           \
    do                                                                  \
    {                                                                   \
        if (diagnostics)                                                \
        {                                                               \
            auto errlog = (const char*)diagnostics->getBufferPointer(); \
            CM_LOG_ERR("[slang diagnostics]: %s", errlog);              \
            APH_ASSERT(false);                                          \
            return {};                                                  \
        }                                                               \
    } while (0)

aph::HashMap<aph::ShaderStage, std::pair<std::string, std::vector<uint32_t>>> loadSlangFromFile(
    std::string_view filename)
{
    APH_PROFILER_SCOPE();
    // TODO multi global session in different threads
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock{ mtx };
    using namespace slang;
    static Slang::ComPtr<IGlobalSession> globalSession;
    {
        static std::once_flag flag;
        std::call_once(flag, []() { slang::createGlobalSession(globalSession.writeRef()); });
    }

    std::vector<CompilerOptionEntry> compilerOptions{{.name = CompilerOptionName::VulkanUseEntryPointName,
                                                      .value =
                                                          {
                                                              .kind      = CompilerOptionValueKind::Int,
                                                              .intValue0 = 1,
                                                          }},
                                                     {.name = CompilerOptionName::EmitSpirvMethod,
                                                      .value{
                                                          .kind      = CompilerOptionValueKind::Int,
                                                          .intValue0 = SLANG_EMIT_SPIRV_DIRECTLY,
                                                      }}};

    TargetDesc targetDesc;
    targetDesc.format = SLANG_SPIRV;
    targetDesc.profile = globalSession->findProfile("spirv");

    targetDesc.compilerOptionEntryCount = compilerOptions.size();
    targetDesc.compilerOptionEntries = compilerOptions.data();

    SessionDesc sessionDesc;
    sessionDesc.targets = &targetDesc;
    sessionDesc.targetCount = 1;

    const char* searchPath = aph::Filesystem::GetInstance().resolvePath("shader_slang://").c_str();
    sessionDesc.searchPaths = &searchPath;
    sessionDesc.searchPathCount = 1;

    Slang::ComPtr<ISession> session;
    auto result = globalSession->createSession(sessionDesc, session.writeRef());
    APH_ASSERT(SLANG_SUCCEEDED(result));

    // PreprocessorMacroDesc fancyFlag = { "ENABLE_FANCY_FEATURE", "1" };
    // sessionDesc.preprocessorMacros = &fancyFlag;
    // sessionDesc.preprocessorMacroCount = 1;

    Slang::ComPtr<IBlob> diagnostics;

    auto fname = aph::Filesystem::GetInstance().resolvePath(filename);
    auto module = session->loadModule(fname.c_str(), diagnostics.writeRef());

    SLANG_CR(diagnostics);

    aph::HashMap<aph::ShaderStage, std::pair<std::string, std::vector<uint32_t>>> spvCodes;

    std::vector<Slang::ComPtr<slang::IComponentType>> componentsToLink;

    for (int i = 0; i < module->getDefinedEntryPointCount(); i++)
    {
        Slang::ComPtr<slang::IEntryPoint> entryPoint;
        result = module->getDefinedEntryPoint(i, entryPoint.writeRef());
        APH_ASSERT(SLANG_SUCCEEDED(result));

        componentsToLink.push_back(Slang::ComPtr<slang::IComponentType>(entryPoint.get()));
    }

    Slang::ComPtr<slang::IComponentType> composed;
    result =
        session->createCompositeComponentType((slang::IComponentType**)componentsToLink.data(), componentsToLink.size(),
                                              composed.writeRef(), diagnostics.writeRef());
    APH_ASSERT(SLANG_SUCCEEDED(result));

    Slang::ComPtr<slang::IComponentType> program;
    result = composed->link(program.writeRef(), diagnostics.writeRef());

    SLANG_CR(diagnostics);

    slang::ProgramLayout* programLayout = program->getLayout(0, diagnostics.writeRef());

    SLANG_CR(diagnostics);

    if (!programLayout)
    {
        CM_LOG_ERR("Failed to get program layout");
        APH_ASSERT(false);
        return {};
    }

    static const aph::HashMap<SlangStage, aph::ShaderStage> slangStageToShaderStageMap = {
        { SLANG_STAGE_VERTEX, aph::ShaderStage::VS },  { SLANG_STAGE_FRAGMENT, aph::ShaderStage::FS },
        { SLANG_STAGE_COMPUTE, aph::ShaderStage::CS }, { SLANG_STAGE_AMPLIFICATION, aph::ShaderStage::TS },
        { SLANG_STAGE_MESH, aph::ShaderStage::MS },
    };

    for (int entryPointIndex = 0; entryPointIndex < programLayout->getEntryPointCount(); entryPointIndex++)
    {
        EntryPointReflection* entryPointReflection = programLayout->getEntryPointByIndex(entryPointIndex);

        Slang::ComPtr<slang::IBlob> spirvCode;
        {
            result = program->getEntryPointCode(entryPointIndex, 0, spirvCode.writeRef(), diagnostics.writeRef());
            SLANG_CR(diagnostics);
            APH_ASSERT(SLANG_SUCCEEDED(result));
        }

        {
            std::vector<uint32_t> retSpvCode;
            retSpvCode.resize(spirvCode->getBufferSize() / sizeof(retSpvCode[0]));
            std::memcpy(retSpvCode.data(), spirvCode->getBufferPointer(), spirvCode->getBufferSize());

            std::string entryPointName = entryPointReflection->getName();
            aph::ShaderStage stage = slangStageToShaderStageMap.at(entryPointReflection->getStage());

            // TODO use the entrypoint name in loading info
            if (spvCodes.contains(stage))
            {
                CM_LOG_WARN("The shader file %s has mutliple entry point of [%s] stage. \
                            \nThe shader module would use the first one.",
                            filename, aph::vk::utils::toString(stage));
            }
            else
            {
                spvCodes[stage] = { entryPointName, std::move(retSpvCode) };
            }
        }
    }

    return spvCodes;
}

#undef SLANG_CR

} // namespace aph::loader::shader

namespace aph::loader::geometry
{
inline bool loadGLTF(aph::ResourceLoader* pLoader, const aph::GeometryLoadInfo& info, aph::Geometry** ppGeometry)
{
    APH_PROFILER_SCOPE();
    auto path = std::filesystem::path{ info.path };
    auto ext = path.extension();

    bool fileLoaded = false;
    tinygltf::Model inputModel;
    tinygltf::TinyGLTF gltfContext;
    std::string error, warning;

    if (ext == ".glb")
    {
        fileLoaded = gltfContext.LoadBinaryFromFile(&inputModel, &error, &warning, path);
    }
    else if (ext == ".gltf")
    {
        fileLoaded = gltfContext.LoadASCIIFromFile(&inputModel, &error, &warning, path);
    }

    if (!fileLoaded)
    {
        CM_LOG_ERR("%s", error);
        return false;
    }

    // TODO gltf loading
    *ppGeometry = new aph::Geometry;

    // Iterate over each mesh
    uint32_t vertexCount = 0;
    for (const auto& mesh : inputModel.meshes)
    {
        for (const auto& primitive : mesh.primitives)
        {
            // Index buffer
            const tinygltf::Accessor& indexAccessor = inputModel.accessors[primitive.indices];
            const tinygltf::BufferView& indexBufferView = inputModel.bufferViews[indexAccessor.bufferView];
            const tinygltf::Buffer& indexBuffer = inputModel.buffers[indexBufferView.buffer];

            {
                aph::vk::Buffer* pIB;
                aph::BufferLoadInfo loadInfo{ .data = (void*)(indexBuffer.data.data() + indexBufferView.byteOffset),
                                              // TODO index type
                                              .createInfo = {
                                                  .size = static_cast<uint32_t>(indexAccessor.count * sizeof(uint16_t)),
                                                  .usage = ::vk::BufferUsageFlagBits::eIndexBuffer } };
                APH_VR(pLoader->load(loadInfo, &pIB));
                (*ppGeometry)->indexBuffer.push_back(pIB);
            }

            // Vertex buffers
            for (const auto& attrib : primitive.attributes)
            {
                const tinygltf::Accessor& accessor = inputModel.accessors[attrib.second];
                const tinygltf::BufferView& bufferView = inputModel.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = inputModel.buffers[bufferView.buffer];

                vertexCount += accessor.count;

                aph::vk::Buffer* pVB;
                aph::BufferLoadInfo loadInfo{
                    .data = (void*)(buffer.data.data() + bufferView.byteOffset),
                    .createInfo = { .size = static_cast<uint32_t>(accessor.count * accessor.ByteStride(bufferView)),
                                    .usage = ::vk::BufferUsageFlagBits::eVertexBuffer },
                };
                APH_VR(pLoader->load(loadInfo, &pVB));
                (*ppGeometry)->vertexBuffers.push_back(pVB);
                (*ppGeometry)->vertexStrides.push_back(accessor.ByteStride(bufferView));
            }

            // TODO: Load draw arguments, handle materials, optimize geometry etc.

        } // End of iterating through primitives
    } // End of iterating through meshes

    const uint32_t indexStride = vertexCount > UINT16_MAX ? sizeof(uint32_t) : sizeof(uint16_t);
    (*ppGeometry)->indexType = (sizeof(uint32_t) == indexStride) ? aph::IndexType::UINT16 : aph::IndexType::UINT32;

    return true;
}

} // namespace aph::loader::geometry

namespace aph
{
ImageContainerType GetImageContainerType(const std::filesystem::path& path)
{
    APH_PROFILER_SCOPE();
    if (path.extension() == ".ktx")
    {
        return ImageContainerType::Ktx;
    }

    if (path.extension() == ".png")
    {
        return ImageContainerType::Png;
    }

    if (path.extension() == ".jpg")
    {
        return ImageContainerType::Jpg;
    }

    CM_LOG_ERR("Unsupported image format.");

    return ImageContainerType::Default;
}

ResourceLoader::ResourceLoader(const ResourceLoaderCreateInfo& createInfo)
    : m_createInfo(createInfo)
    , m_pDevice(createInfo.pDevice)
{
    m_pQueue = m_pDevice->getQueue(QueueType::Transfer);
}

ResourceLoader::~ResourceLoader() = default;

void ResourceLoader::cleanup()
{
    APH_PROFILER_SCOPE();
    for (const auto& [_, shaderCache] : m_shaderCaches)
    {
        for (const auto& [_, shader] : shaderCache)
        {
            m_pDevice->destroy(shader);
        }
    }
}

Result ResourceLoader::load(const ImageLoadInfo& info, vk::Image** ppImage)
{
    APH_PROFILER_SCOPE();
    std::filesystem::path path;
    std::vector<uint8_t> data;
    vk::ImageCreateInfo ci;
    ci = info.createInfo;

    if (std::holds_alternative<std::string>(info.data))
    {
        path = aph::Filesystem::GetInstance().resolvePath(std::get<std::string>(info.data));

        auto containerType =
            info.containerType == ImageContainerType::Default ? GetImageContainerType(path) : info.containerType;

        switch (containerType)
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
            return { Result::RuntimeError, "Unsupported image type." };
        }
    }
    else if (std::holds_alternative<ImageInfo>(info.data))
    {
        auto img = std::get<ImageInfo>(info.data);
        data = img.data;
        ci.extent = { img.width, img.height, 1 };
    }

    // Load texture from image buffer
    vk::Buffer* stagingBuffer;
    {
        vk::BufferCreateInfo bufferCI{
            .size = static_cast<uint32_t>(data.size()),
            .usage = ::vk::BufferUsageFlagBits::eTransferSrc,
            .domain = MemoryDomain::Upload,
        };
        APH_VR(m_pDevice->create(bufferCI, &stagingBuffer, std::string{ info.debugName } + std::string{ "_staging" }));

        writeBuffer(stagingBuffer, data.data());
    }

    vk::Image* image{};

    {
        bool genMipmap = ci.mipLevels > 1;

        auto imageCI = ci;
        imageCI.usage |= ::vk::ImageUsageFlagBits::eTransferDst;
        imageCI.domain = MemoryDomain::Device;
        if (genMipmap)
        {
            imageCI.usage |= ::vk::ImageUsageFlagBits::eTransferSrc;
        }

        APH_VR(m_pDevice->create(imageCI, &image, info.debugName));

        auto queue = m_pQueue;

        // mip map opeartions
        m_pDevice->executeCommand(
            queue,
            [&](auto* cmd)
            {
                cmd->transitionImageLayout(image, aph::ResourceState::CopyDest);
                cmd->copy(stagingBuffer, image);

                if (genMipmap)
                {
                    cmd->transitionImageLayout(image, aph::ResourceState::CopySource);
                    int32_t width = ci.extent.width;
                    int32_t height = ci.extent.height;

                    // generate mipmap chains
                    for (int32_t i = 1; i < imageCI.mipLevels; i++)
                    {
                        vk::ImageBlitInfo srcBlitInfo{
                            .extent = { int32_t(width >> (i - 1)), int32_t(height >> (i - 1)), 1 },
                            .level = static_cast<uint32_t>(i - 1),
                            .layerCount = 1,
                        };

                        vk::ImageBlitInfo dstBlitInfo{
                            .extent = { int32_t(width >> i), int32_t(height >> i), 1 },
                            .level = static_cast<uint32_t>(i),
                            .layerCount = 1,
                        };

                        // Prepare current mip level as image blit destination
                        vk::ImageBarrier barrier{
                            .pImage = image,
                            .currentState = image->getResourceState(),
                            .newState = ResourceState::CopyDest,
                            .subresourceBarrier = 1,
                            .mipLevel = static_cast<uint8_t>(imageCI.mipLevels),
                        };
                        cmd->insertBarrier({ barrier });

                        // Blit from previous level
                        cmd->blit(image, image, srcBlitInfo, dstBlitInfo);

                        barrier.currentState = image->getResourceState();
                        barrier.newState = ResourceState::CopySource;
                        cmd->insertBarrier({ barrier });
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
        bufferCI.usage |= ::vk::BufferUsageFlagBits::eTransferDst;
        APH_VR(m_pDevice->create(bufferCI, ppBuffer, info.debugName));
    }

    // update buffer
    if (info.data)
    {
        this->update(
            {
                .data = info.data,
                .range = { 0, info.createInfo.size },
            },
            ppBuffer);
    }

    return Result::Success;
}

Result ResourceLoader::load(const ShaderLoadInfo& info, vk::ShaderProgram** ppProgram)
{
    APH_PROFILER_SCOPE();

    auto loadShader = [this](const std::vector<uint32_t>& spv, const aph::ShaderStage stage,
                             const std::string& entryPoint = "main") -> vk::Shader*
    {
        vk::Shader* shader;
        vk::ShaderCreateInfo createInfo{
            .code = spv,
            .entrypoint = entryPoint,
            .stage = stage,
        };
        APH_VR(m_pDevice->create(createInfo, &shader));
        return shader;
    };

    HashMap<ShaderStage, vk::Shader*> requiredShaderList;
    HashMap<std::filesystem::path, HashMap<aph::ShaderStage, std::string>> requiredStageMaps;
    for (auto& [stage, stageLoadInfo] : info.stageInfo)
    {
        if (std::holds_alternative<std::string>(stageLoadInfo.data))
        {
            auto path = Filesystem::GetInstance().resolvePath(std::get<std::string>(stageLoadInfo.data));
            requiredStageMaps[path][stage] = stageLoadInfo.entryPoint;
        }
        else
        {
            requiredShaderList[stage] =
                loadShader(std::get<std::vector<uint32_t>>(stageLoadInfo.data), stage, stageLoadInfo.entryPoint);
        }
    }

    for (const auto& [path, requiredStages] : requiredStageMaps)
    {
        if (m_shaderCaches.contains(path.string()))
        {
            const auto& shaderCache = m_shaderCaches[path.string()];
            for (const auto& [stage, entryPoint] : requiredStages)
            {
                APH_ASSERT(!shaderCache.contains(stage));
                requiredShaderList[stage] = shaderCache.at(stage);
            }
            continue;
        }

        if (path.extension() == ".spv")
        {
            // TODO multi shader stage single spv binary support
            ShaderStage stage = requiredStages.cbegin()->first;
            vk::Shader* shader = loadShader(loader::shader::loadSpvFromFile(path.c_str()), stage);
            requiredShaderList[stage] = shader;
            m_shaderCaches[path][stage] = shader;
        }
        else if (path.extension() == ".slang")
        {
            auto spvCodeMap = loader::shader::loadSlangFromFile(path.c_str());
            if (spvCodeMap.empty())
            {
                return { Result::RuntimeError, "Failed to load slang shader from file." };
            }
            for (const auto& [stage, spvInfo] : spvCodeMap)
            {
                const auto& [entryPointName, spv] = spvInfo;
                APH_ASSERT(!requiredShaderList.contains(stage));

                vk::Shader* shader = loadShader(spv, stage, entryPointName);
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
            return { Result::RuntimeError, "Unsupported shader format." };
        }
    }

    // vs + fs
    if (requiredShaderList.contains(ShaderStage::VS) && requiredShaderList.contains(ShaderStage::FS))
    {
        APH_VR(m_pDevice->create(
            vk::ProgramCreateInfo{
                .geometry{ .pVertex = requiredShaderList[ShaderStage::VS],
                           .pFragment = requiredShaderList[ShaderStage::FS] },
                .type = PipelineType::Geometry,
            },
            ppProgram));
    }
    else if (requiredShaderList.contains(ShaderStage::MS) && requiredShaderList.contains(ShaderStage::FS))
    {
        vk::ProgramCreateInfo ci{
            .mesh{ .pMesh = requiredShaderList[ShaderStage::MS], .pFragment = requiredShaderList[ShaderStage::FS] },
            .type = PipelineType::Mesh,
        };
        if (requiredShaderList.contains(ShaderStage::TS))
        {
            ci.mesh.pTask = requiredShaderList[ShaderStage::TS];
        }
        APH_VR(m_pDevice->create(ci, ppProgram));
    }
    // cs
    else if (requiredShaderList.contains(ShaderStage::CS))
    {
        APH_VR(m_pDevice->create(
            vk::ProgramCreateInfo{
                .compute{ .pCompute = requiredShaderList[ShaderStage::CS] },
                .type = PipelineType::Compute,
            },
            ppProgram));
    }
    else
    {
        APH_ASSERT(false);
        return { Result::RuntimeError, "Unsupported shader stage combinations." };
    }

    return Result::Success;
}

Result ResourceLoader::load(const GeometryLoadInfo& info, Geometry** ppGeometry)
{
    APH_PROFILER_SCOPE();
    auto path = std::filesystem::path{ info.path };
    auto ext = path.extension();

    if (ext == ".glb" || ext == ".gltf")
    {
        loader::geometry::loadGLTF(this, info, ppGeometry);
    }
    else
    {
        CM_LOG_ERR("Unsupported model file type: %s.", ext);
        APH_ASSERT(false);
        return { Result::RuntimeError, "Unsupported model file type." };
    }
    return Result::Success;
}

void ResourceLoader::update(const BufferUpdateInfo& info, vk::Buffer** ppBuffer)
{
    APH_PROFILER_SCOPE();
    vk::Buffer* pBuffer = *ppBuffer;
    MemoryDomain domain = pBuffer->getCreateInfo().domain;
    std::size_t uploadSize = info.range.size;

    // device only
    if (domain == MemoryDomain::Device)
    {
        if (info.range.size == VK_WHOLE_SIZE)
        {
            uploadSize = pBuffer->getSize();
        }

        if (uploadSize <= LIMIT_BUFFER_CMD_UPDATE_SIZE)
        {
            APH_PROFILER_SCOPE_NAME("loading data by: vkCmdBufferUpdate.");
            m_pDevice->executeCommand(m_pQueue, [=](auto* cmd) { cmd->update(pBuffer, { 0, uploadSize }, info.data); });
        }
        else
        {
            APH_PROFILER_SCOPE_NAME("loading data by: staging copy.");
            for (std::size_t offset = info.range.offset; offset < uploadSize; offset += LIMIT_BUFFER_UPLOAD_SIZE)
            {
                Range copyRange = {
                    .offset = offset,
                    .size = std::min(std::size_t{ LIMIT_BUFFER_UPLOAD_SIZE }, { uploadSize - offset }),
                };

                // using staging buffer
                vk::Buffer* stagingBuffer{};
                {
                    vk::BufferCreateInfo stagingCI{
                        .size = static_cast<uint32_t>(copyRange.size),
                        .usage = ::vk::BufferUsageFlagBits::eTransferSrc,
                        .domain = MemoryDomain::Upload,
                    };

                    APH_VR(m_pDevice->create(stagingCI, &stagingBuffer, "staging buffer"));

                    writeBuffer(stagingBuffer, info.data, { 0, copyRange.size });
                }

                m_pDevice->executeCommand(m_pQueue, [=](auto* cmd) { cmd->copy(stagingBuffer, pBuffer, copyRange); });

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

void ResourceLoader::writeBuffer(vk::Buffer* pBuffer, const void* data, Range range)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(pBuffer->getCreateInfo().domain != MemoryDomain::Device);
    if (range.size == 0)
    {
        range.size = VK_WHOLE_SIZE;
    }

    if (range.size == VK_WHOLE_SIZE || range.size == 0)
    {
        range.size = pBuffer->getSize();
    }

    void* pMapped = m_pDevice->mapMemory(pBuffer);
    APH_ASSERT(pMapped);
    std::memcpy((uint8_t*)pMapped + range.offset, data, range.size);
    m_pDevice->unMapMemory(pBuffer);
}
} // namespace aph
