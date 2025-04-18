#include "geometryLoader.h"
#include "filesystem/filesystem.h"
#include "geometry/meshletBuilder.h"
#include "resource/resourceLoader.h"
#include "stb_image.h"

#include "common/profiler.h"
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

#include <meshoptimizer.h>

namespace aph
{

GeometryLoader::GeometryLoader(ResourceLoader* pResourceLoader)
    : m_pResourceLoader(pResourceLoader)
{
}

GeometryLoader::~GeometryLoader() = default;

auto GeometryLoader::load(const GeometryLoadInfo& info, GeometryAsset** ppGeometryAsset) -> Result
{
    APH_PROFILER_SCOPE();

    *ppGeometryAsset = m_geometryAssetPool.allocate();

    auto path = std::filesystem::path{ APH_DEFAULT_FILESYSTEM.resolvePath(info.path) };
    auto ext  = path.extension();

    // Handle different file formats
    if (ext == ".glb" || ext == ".gltf")
    {
        return loadGLTF(info, ppGeometryAsset);
    }

    // Unsupported format
    unload(*ppGeometryAsset);
    *ppGeometryAsset = nullptr;

    return { Result::RuntimeError, "Unsupported geometry file format: " + std::string(ext.c_str()) };
}

void GeometryLoader::unload(GeometryAsset* pGeometryAsset)
{
    if (pGeometryAsset != nullptr)
    {
        m_geometryAssetPool.free(pGeometryAsset);
    }
}

auto GeometryLoader::loadGLTF(const GeometryLoadInfo& info, GeometryAsset** ppGeometryAsset) -> Result
{
    APH_PROFILER_SCOPE();

    // Load the GLTF file
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string error;
    std::string warning;

    bool success = false;

    auto path = std::filesystem::path{ APH_DEFAULT_FILESYSTEM.resolvePath(info.path) };
    auto ext  = path.extension();

    if (ext == ".glb")
    {
        success = loader.LoadBinaryFromFile(&model, &error, &warning, path);
    }
    else if (ext == ".gltf")
    {
        success = loader.LoadASCIIFromFile(&model, &error, &warning, path);
    }

    if (!success)
    {
        return { Result::RuntimeError, "Failed to load GLTF model: " + error };
    }

    if (!warning.empty())
    {
        LOADER_LOG_WARN("GLTF loader warning: %s", warning.c_str());
    }

    // Process the model into our internal mesh format
    std::vector<GLTFMesh> meshes;

    for (const auto& gltfMesh : model.meshes)
    {
        for (const auto& primitive : gltfMesh.primitives)
        {
            // Skip if not triangles
            if (primitive.mode != TINYGLTF_MODE_TRIANGLES)
            {
                continue;
            }

            GLTFMesh mesh;
            mesh.materialIndex = static_cast<uint32_t>(primitive.material);

            // Process indices
            if (primitive.indices >= 0)
            {
                const auto& accessor   = model.accessors[static_cast<size_t>(primitive.indices)];
                const auto& bufferView = model.bufferViews[static_cast<size_t>(accessor.bufferView)];
                const auto& buffer     = model.buffers[static_cast<size_t>(bufferView.buffer)];

                mesh.indices.resize(static_cast<size_t>(accessor.count));

                // Handle different index types
                if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                {
                    std::span<const uint16_t> indices{ reinterpret_cast<const uint16_t*>(buffer.data.data() +
                                                                                         bufferView.byteOffset +
                                                                                         accessor.byteOffset),
                                                       static_cast<size_t>(accessor.count) };

                    std::ranges::transform(indices, mesh.indices.begin(),
                                           [](uint16_t idx) -> uint32_t
                                           {
                                               return static_cast<uint32_t>(idx);
                                           });
                }
                else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                {
                    std::span<const uint32_t> indices{ reinterpret_cast<const uint32_t*>(buffer.data.data() +
                                                                                         bufferView.byteOffset +
                                                                                         accessor.byteOffset),
                                                       static_cast<size_t>(accessor.count) };

                    std::ranges::copy(indices, mesh.indices.begin());
                }
                else
                {
                    return { Result::RuntimeError, "Unsupported index component type" };
                }
            }

            // Process vertex attributes
            for (const auto& [attrName, attrIndex] : primitive.attributes)
            {
                const auto& accessor   = model.accessors[static_cast<size_t>(attrIndex)];
                const auto& bufferView = model.bufferViews[static_cast<size_t>(accessor.bufferView)];
                const auto& buffer     = model.buffers[static_cast<size_t>(bufferView.buffer)];

                const uint8_t* dataPtr = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;

                const std::string_view attributeName{ attrName };

                if (attributeName == "POSITION" && accessor.type == TINYGLTF_TYPE_VEC3 &&
                    accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                {
                    mesh.positions.resize(static_cast<size_t>(accessor.count) * 3);
                    std::span<const float> positions{ reinterpret_cast<const float*>(dataPtr),
                                                      static_cast<size_t>(accessor.count) * 3 };
                    std::ranges::copy(positions, mesh.positions.begin());
                }
                else if (attributeName == "NORMAL" && accessor.type == TINYGLTF_TYPE_VEC3 &&
                         accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                {
                    mesh.normals.resize(static_cast<size_t>(accessor.count) * 3);
                    std::span<const float> normals{ reinterpret_cast<const float*>(dataPtr),
                                                    static_cast<size_t>(accessor.count) * 3 };
                    std::ranges::copy(normals, mesh.normals.begin());
                }
                else if (attributeName == "TANGENT" && accessor.type == TINYGLTF_TYPE_VEC4 &&
                         accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                {
                    mesh.tangents.resize(static_cast<size_t>(accessor.count) * 4);
                    std::span<const float> tangents{ reinterpret_cast<const float*>(dataPtr),
                                                     static_cast<size_t>(accessor.count) * 4 };
                    std::ranges::copy(tangents, mesh.tangents.begin());
                }
                else if (attributeName == "TEXCOORD_0" && accessor.type == TINYGLTF_TYPE_VEC2 &&
                         accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                {
                    mesh.texcoords0.resize(static_cast<size_t>(accessor.count) * 2);
                    std::span<const float> texcoords{ reinterpret_cast<const float*>(dataPtr),
                                                      static_cast<size_t>(accessor.count) * 2 };
                    std::ranges::copy(texcoords, mesh.texcoords0.begin());
                }
                else if (attributeName == "TEXCOORD_1" && accessor.type == TINYGLTF_TYPE_VEC2 &&
                         accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                {
                    mesh.texcoords1.resize(static_cast<size_t>(accessor.count) * 2);
                    std::span<const float> texcoords{ reinterpret_cast<const float*>(dataPtr),
                                                      static_cast<size_t>(accessor.count) * 2 };
                    std::ranges::copy(texcoords, mesh.texcoords1.begin());
                }
                else if (attributeName == "COLOR_0")
                {
                    if (accessor.type == TINYGLTF_TYPE_VEC3 && accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                    {
                        mesh.colors.resize(static_cast<size_t>(accessor.count) * 3);
                        std::span<const float> colors{ reinterpret_cast<const float*>(dataPtr),
                                                       static_cast<size_t>(accessor.count) * 3 };
                        std::ranges::copy(colors, mesh.colors.begin());
                    }
                    else if (accessor.type == TINYGLTF_TYPE_VEC4 &&
                             accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                    {
                        mesh.colors.resize(static_cast<size_t>(accessor.count) * 4);
                        std::span<const float> colors{ reinterpret_cast<const float*>(dataPtr),
                                                       static_cast<size_t>(accessor.count) * 4 };
                        std::ranges::copy(colors, mesh.colors.begin());
                    }
                }
            }

            // Generate normals if needed and not present
            if (mesh.normals.empty() && (info.attributeFlags & GeometryAttributeBits::eGenerateNormals))
            {
                const size_t vertexCount = mesh.positions.size() / 3;
                mesh.normals.resize(vertexCount * 3, 0.0f); // Initialize with zeros

                // Generate flat normals
                for (size_t i = 0; i < mesh.indices.size(); i += 3)
                {
                    // Safe access with at() instead of operator[]
                    const uint32_t i0 = mesh.indices.at(i);
                    const uint32_t i1 = mesh.indices.at(i + 1);
                    const uint32_t i2 = mesh.indices.at(i + 2);

                    // Safe access to position data
                    const size_t i0Base = static_cast<size_t>(i0) * 3;
                    const size_t i1Base = static_cast<size_t>(i1) * 3;
                    const size_t i2Base = static_cast<size_t>(i2) * 3;

                    const std::array<float, 3> v0 = { mesh.positions.at(i0Base), mesh.positions.at(i0Base + 1),
                                                      mesh.positions.at(i0Base + 2) };

                    const std::array<float, 3> v1 = { mesh.positions.at(i1Base), mesh.positions.at(i1Base + 1),
                                                      mesh.positions.at(i1Base + 2) };

                    const std::array<float, 3> v2 = { mesh.positions.at(i2Base), mesh.positions.at(i2Base + 1),
                                                      mesh.positions.at(i2Base + 2) };

                    // Calculate cross product for face normal: (v1-v0) × (v2-v0)
                    // Cross product components:
                    // nx = (v1.y - v0.y)*(v2.z - v0.z) - (v1.z - v0.z)*(v2.y - v0.y)
                    // ny = (v1.z - v0.z)*(v2.x - v0.x) - (v1.x - v0.x)*(v2.z - v0.z)
                    // nz = (v1.x - v0.x)*(v2.y - v0.y) - (v1.y - v0.y)*(v2.x - v0.x)

                    const float e1y = v1[1] - v0[1];
                    const float e1z = v1[2] - v0[2];
                    const float e2y = v2[1] - v0[1];
                    const float e2z = v2[2] - v0[2];
                    const float e1x = v1[0] - v0[0];
                    const float e2x = v2[0] - v0[0];

                    const float nx = (e1y * e2z) - (e1z * e2y);
                    const float ny = (e1z * e2x) - (e1x * e2z);
                    const float nz = (e1x * e2y) - (e1y * e2x);

                    // Compute length for normalization
                    const float lenSquared = (nx * nx) + (ny * ny) + (nz * nz);
                    const float len        = std::sqrt(lenSquared);

                    // Create normalized normal
                    const auto [normalized_nx, normalized_ny, normalized_nz] = [len, nx, ny,
                                                                                nz]() -> std::tuple<float, float, float>
                    {
                        if (len > 0.0f)
                        {
                            const float invLen = 1.0f / len;
                            return std::tuple<float, float, float>{ nx * invLen, ny * invLen, nz * invLen };
                        }
                        return std::tuple<float, float, float>{ 0.0f, 0.0f, 0.0f };
                    }();

                    // Add to all vertices
                    for (const auto& vertexIndex : { i0, i1, i2 })
                    {
                        const size_t normalBase = static_cast<size_t>(vertexIndex) * 3;
                        mesh.normals.at(normalBase) += normalized_nx;
                        mesh.normals.at(normalBase + 1) += normalized_ny;
                        mesh.normals.at(normalBase + 2) += normalized_nz;
                    }
                }

                // Normalize all normals
                for (size_t i = 0; i < vertexCount; ++i)
                {
                    const size_t baseIdx = i * 3;
                    const float nx       = mesh.normals.at(baseIdx);
                    const float ny       = mesh.normals.at(baseIdx + 1);
                    const float nz       = mesh.normals.at(baseIdx + 2);

                    const float lenSquared = (nx * nx) + (ny * ny) + (nz * nz);
                    const float len        = std::sqrt(lenSquared);

                    if (len > 0.0f)
                    {
                        const float invLen           = 1.0f / len;
                        mesh.normals.at(baseIdx)     = nx * invLen;
                        mesh.normals.at(baseIdx + 1) = ny * invLen;
                        mesh.normals.at(baseIdx + 2) = nz * invLen;
                    }
                }
            }

            // Add to our list of meshes
            meshes.push_back(std::move(mesh));
        }
    }

    // Process the meshes with meshoptimizer
    return processGeometry(meshes, info, ppGeometryAsset);
}

auto GeometryLoader::processGeometry(const std::vector<GLTFMesh>& meshes, const GeometryLoadInfo& info,
                                     GeometryAsset** ppGeometryAsset) -> Result
{
    APH_PROFILER_SCOPE();

    if (meshes.empty())
    {
        return { Result::RuntimeError, "No valid meshes found in the model" };
    }

    // Build meshlets from the geometry
    MeshletBuilder meshletBuilder;

    // Add each mesh to the builder
    for (const auto& mesh : meshes)
    {
        // Skip empty meshes
        if (mesh.positions.empty() || mesh.indices.empty())
        {
            continue;
        }

        const auto positionStride = static_cast<uint32_t>(3 * sizeof(float));
        const auto normalStride =
            mesh.normals.empty() ? static_cast<uint32_t>(0) : static_cast<uint32_t>(3 * sizeof(float));

        meshletBuilder.addMesh(mesh.positions.data(), positionStride, static_cast<uint32_t>(mesh.positions.size() / 3),
                               mesh.indices.data(), static_cast<uint32_t>(mesh.indices.size()),
                               mesh.normals.empty() ? nullptr : mesh.normals.data(), normalStride);
    }

    // Determine build flags from optimization flags
    const bool optimizeOverdraw =
        (info.optimizationFlags & GeometryOptimizationBits::eOverdraw) != GeometryOptimizationBits::eNone;
    const bool optimizeVertexFetch =
        (info.optimizationFlags & GeometryOptimizationBits::eVertexFetch) != GeometryOptimizationBits::eNone;

    // Build the meshlets with the requested parameters
    meshletBuilder.build(info.maxVertsPerMeshlet, info.maxPrimsPerMeshlet, optimizeOverdraw, optimizeVertexFetch);

    // Extract the meshlet data
    std::vector<Meshlet> meshlets;
    std::vector<uint32_t> meshletVertices;
    std::vector<uint32_t> meshletIndices;

    meshletBuilder.exportMeshletData(meshlets, meshletVertices, meshletIndices);

    // Create submeshes
    std::vector<Submesh> submeshes = meshletBuilder.generateSubmeshes();

    // Create GPU resources for the geometry
    return createGeometryResources(meshlets, meshletVertices, meshletIndices, submeshes, meshes, info, ppGeometryAsset);
}

auto GeometryLoader::createGeometryResources(const std::vector<Meshlet>& meshlets,
                                             const std::vector<uint32_t>& meshletVertices,
                                             const std::vector<uint32_t>& meshletIndices,
                                             const std::vector<Submesh>& submeshes, const std::vector<GLTFMesh>& meshes,
                                             const GeometryLoadInfo& info, GeometryAsset** ppGeometryAsset) -> Result
{
    APH_PROFILER_SCOPE();

    // First, let's collect all vertex data for the entire mesh
    struct VertexData
    {
        std::array<float, 3> position{};
        std::array<float, 3> normal{};
        std::array<float, 2> texcoord{};
    };

    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;

    // Build a vertex buffer with all of our data
    for (const auto& mesh : meshes)
    {
        if (mesh.positions.empty())
        {
            continue;
        }

        const size_t baseVertex  = vertices.size();
        const size_t vertexCount = mesh.positions.size() / 3;

        // Create vertices
        const auto oldSize = vertices.size();
        vertices.resize(oldSize + vertexCount);

        for (size_t i = 0; i < vertexCount; ++i)
        {
            auto& v = vertices.at(baseVertex + i);

            // Position (always available) - scale from [-1,1] to [-0.5,0.5] range
            const size_t posBaseIdx = i * 3;
            v.position[0]           = mesh.positions.at(posBaseIdx) * 0.5f;
            v.position[1]           = mesh.positions.at(posBaseIdx + 1) * 0.5f;
            v.position[2]           = mesh.positions.at(posBaseIdx + 2) * 0.5f;

            // Normal (might be empty)
            if ((posBaseIdx + 2) < mesh.normals.size())
            {
                v.normal[0] = mesh.normals.at(posBaseIdx);
                v.normal[1] = mesh.normals.at(posBaseIdx + 1);
                v.normal[2] = mesh.normals.at(posBaseIdx + 2);
            }
            else
            {
                v.normal = { 0.0f, 1.0f, 0.0f }; // Default up normal
            }

            // Texcoord (might be empty)
            const size_t texBaseIdx = i * 2;
            if ((texBaseIdx + 1) < mesh.texcoords0.size())
            {
                v.texcoord[0] = mesh.texcoords0.at(texBaseIdx);
                v.texcoord[1] = mesh.texcoords0.at(texBaseIdx + 1);
            }
            else
            {
                v.texcoord = { 0.0f, 0.0f }; // Default UV
            }
        }

        // Create indices (adjust for base vertex offset)
        const size_t baseIndex = indices.size();
        indices.resize(baseIndex + mesh.indices.size());

        // Transform indices to account for the base vertex offset
        std::ranges::transform(mesh.indices, indices.begin() + static_cast<std::ptrdiff_t>(baseIndex),
                               [baseVertex](uint32_t idx) -> uint32_t
                               {
                                   return static_cast<uint32_t>(baseVertex) + idx;
                               });
    }

    // Create separate position and attribute buffers to match expected shader format
    // The shader expects Vec4 positions and Vec2 UVs
    std::vector<Vec4> positionData;
    std::vector<Vec2> attributeData;

    positionData.reserve(vertices.size());
    attributeData.reserve(vertices.size());

    // Transform vertex data into the format expected by the shader
    std::ranges::transform(vertices, std::back_inserter(positionData),
                           [](const VertexData& v) -> Vec4
                           {
                               return Vec4{ v.position[0], v.position[1], v.position[2], 1.0f };
                           });

    std::ranges::transform(vertices, std::back_inserter(attributeData),
                           [](const VertexData& v) -> Vec2
                           {
                               return Vec2{ v.texcoord[0], v.texcoord[1] };
                           });

    // Create GPU data structure
    GeometryGpuData gpuData{};
    gpuData.vertexCount             = static_cast<uint32_t>(vertices.size());
    gpuData.indexCount              = static_cast<uint32_t>(indices.size());
    gpuData.meshletCount            = static_cast<uint32_t>(meshlets.size());
    gpuData.meshletMaxVertexCount   = info.maxVertsPerMeshlet;
    gpuData.meshletMaxTriangleCount = info.maxPrimsPerMeshlet;

    // Determine index type based on vertex count
    gpuData.indexType = (vertices.size() > static_cast<size_t>(UINT16_MAX)) ? IndexType::UINT32 : IndexType::UINT16;

    // Create position buffer
    {
        BufferLoadInfo bufferInfo{
            .debugName   = info.debugName + "::position_buffer",
            .data        = positionData.data(),
            .dataSize    = positionData.size() * sizeof(Vec4),
            .createInfo  = { .size   = static_cast<uint32_t>(positionData.size() * sizeof(Vec4)),
                            .usage  = BufferUsage::Vertex | BufferUsage::Storage,
                            .domain = MemoryDomain::Device },
            .contentType = BufferContentType::Vertex
        };

        auto expected = m_pResourceLoader->load(bufferInfo);
        VerifyExpected(expected);
        gpuData.pPositionBuffer = expected.value()->getBuffer();
    }

    // Create attribute buffer
    {
        BufferLoadInfo bufferInfo{
            .debugName   = info.debugName + "::attribute_buffer",
            .data        = attributeData.data(),
            .dataSize    = attributeData.size() * sizeof(Vec2),
            .createInfo  = { .size   = static_cast<uint32_t>(attributeData.size() * sizeof(Vec2)),
                            .usage  = BufferUsage::Vertex | BufferUsage::Storage,
                            .domain = MemoryDomain::Device },
            .contentType = BufferContentType::Vertex
        };

        auto expected = m_pResourceLoader->load(bufferInfo);
        VerifyExpected(expected);
        gpuData.pAttributeBuffer = expected.value()->getBuffer();
    }

    // Create index buffer
    {
        if (gpuData.indexType == IndexType::UINT16)
        {
            // Convert to uint16_t indices
            std::vector<uint16_t> indices16;
            indices16.reserve(indices.size());

            std::ranges::transform(indices, std::back_inserter(indices16),
                                   [](uint32_t idx) -> uint16_t
                                   {
                                       return static_cast<uint16_t>(idx);
                                   });

            BufferLoadInfo bufferInfo{
                .debugName   = info.debugName + "::index_buffer",
                .data        = indices16.data(),
                .dataSize    = indices16.size() * sizeof(uint16_t),
                .createInfo  = { .size   = static_cast<uint32_t>(indices16.size() * sizeof(uint16_t)),
                                .usage  = BufferUsage::Index | BufferUsage::Storage,
                                .domain = MemoryDomain::Device },
                .contentType = BufferContentType::Index
            };

            auto expected = m_pResourceLoader->load(bufferInfo);
            VerifyExpected(expected);
            gpuData.pIndexBuffer = expected.value()->getBuffer();
        }
        else
        {
            BufferLoadInfo bufferInfo{
                .debugName   = info.debugName + "::index_buffer",
                .data        = indices.data(),
                .dataSize    = indices.size() * sizeof(uint32_t),
                .createInfo  = { .size   = static_cast<uint32_t>(indices.size() * sizeof(uint32_t)),
                                .usage  = BufferUsage::Index | BufferUsage::Storage,
                                .domain = MemoryDomain::Device },
                .contentType = BufferContentType::Index
            };

            auto expected = m_pResourceLoader->load(bufferInfo);
            VerifyExpected(expected);
            gpuData.pIndexBuffer = expected.value()->getBuffer();
        }
    }

    // Create meshlet buffer
    {
        BufferLoadInfo bufferInfo{
            .debugName   = info.debugName + "::meshlet_buffer",
            .data        = meshlets.data(),
            .dataSize    = meshlets.size() * sizeof(Meshlet),
            .createInfo  = { .size   = static_cast<uint32_t>(meshlets.size() * sizeof(Meshlet)),
                            .usage  = BufferUsage::Storage,
                            .domain = MemoryDomain::Device },
            .contentType = BufferContentType::Storage
        };

        auto expected = m_pResourceLoader->load(bufferInfo);
        VerifyExpected(expected);
        gpuData.pMeshletBuffer = expected.value()->getBuffer();
    }

    // Create meshlet vertex buffer
    {
        BufferLoadInfo bufferInfo{
            .debugName   = info.debugName + "::meshlet_vertex_buffer",
            .data        = meshletVertices.data(),
            .dataSize    = meshletVertices.size() * sizeof(uint32_t),
            .createInfo  = { .size   = static_cast<uint32_t>(meshletVertices.size() * sizeof(uint32_t)),
                            .usage  = BufferUsage::Storage,
                            .domain = MemoryDomain::Device },
            .contentType = BufferContentType::Storage
        };

        auto expected = m_pResourceLoader->load(bufferInfo);
        VerifyExpected(expected);
        gpuData.pMeshletVertexBuffer = expected.value()->getBuffer();
    }

    // Create meshlet index buffer
    {
        BufferLoadInfo bufferInfo{
            .debugName   = info.debugName + "::meshlet_index_buffer",
            .data        = meshletIndices.data(),
            .dataSize    = meshletIndices.size() * sizeof(uint32_t),
            .createInfo  = { .size   = static_cast<uint32_t>(meshletIndices.size() * sizeof(uint32_t)),
                            .usage  = BufferUsage::Storage,
                            .domain = MemoryDomain::Device },
            .contentType = BufferContentType::Storage
        };

        auto expected = m_pResourceLoader->load(bufferInfo);
        VerifyExpected(expected);
        gpuData.pMeshletIndexBuffer = expected.value()->getBuffer();
    }

    // Create the geometry resource
    auto* pDevice = m_pResourceLoader->getDevice();

    auto pGeometryResource = GeometryResourceFactory::createGeometryResource(pDevice, gpuData, submeshes,
                                                                             info.vertexInput, info.preferMeshShading);

    // Set the geometry resource in the asset
    (*ppGeometryAsset)->setGeometryResource(std::move(pGeometryResource));

    return Result::Success;
}

} // namespace aph
