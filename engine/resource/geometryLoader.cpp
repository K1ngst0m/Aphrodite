#include "common/profiler.h"

#include "resource/resourceLoader.h"
#include "stb/stb_image.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

namespace aph::loader::geometry
{

bool loadGLTF(aph::ResourceLoader* pLoader, const aph::GeometryLoadInfo& info, aph::Geometry** ppGeometry)
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
                                                  .usage = BufferUsage::Index } };
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
                                    .usage = BufferUsage::Vertex },
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
