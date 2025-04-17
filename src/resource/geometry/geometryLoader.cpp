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
            mesh.materialIndex = primitive.material;

            // Process indices
            if (primitive.indices >= 0)
            {
                const tinygltf::Accessor& accessor     = model.accessors[primitive.indices];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer         = model.buffers[bufferView.buffer];

                mesh.indices.resize(accessor.count);

                // Handle different index types
                if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                {
                    const auto* indices = reinterpret_cast<const uint16_t*>(buffer.data.data() + bufferView.byteOffset +
                                                                            accessor.byteOffset);

                    for (size_t i = 0; i < accessor.count; ++i)
                    {
                        mesh.indices[i] = static_cast<uint32_t>(indices[i]);
                    }
                }
                else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                {
                    const auto* indices = reinterpret_cast<const uint32_t*>(buffer.data.data() + bufferView.byteOffset +
                                                                            accessor.byteOffset);

                    std::memcpy(mesh.indices.data(), indices, accessor.count * sizeof(uint32_t));
                }
                else
                {
                    return { Result::RuntimeError, "Unsupported index component type" };
                }
            }

            // Process vertex attributes
            for (const auto& attr : primitive.attributes)
            {
                const tinygltf::Accessor& accessor     = model.accessors[attr.second];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer         = model.buffers[bufferView.buffer];

                const uint8_t* dataPtr = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;

                if (attr.first == "POSITION")
                {
                    mesh.positions.resize(accessor.count * 3);
                    if (accessor.type == TINYGLTF_TYPE_VEC3)
                    {
                        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                        {
                            const float* positions = reinterpret_cast<const float*>(dataPtr);
                            for (size_t i = 0; i < accessor.count; ++i)
                            {
                                mesh.positions[i * 3 + 0] = positions[i * 3 + 0];
                                mesh.positions[i * 3 + 1] = positions[i * 3 + 1];
                                mesh.positions[i * 3 + 2] = positions[i * 3 + 2];
                            }
                        }
                    }
                }
                else if (attr.first == "NORMAL")
                {
                    mesh.normals.resize(accessor.count * 3);
                    if (accessor.type == TINYGLTF_TYPE_VEC3)
                    {
                        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                        {
                            const auto* normals = reinterpret_cast<const float*>(dataPtr);
                            for (size_t i = 0; i < accessor.count; ++i)
                            {
                                mesh.normals[i * 3 + 0] = normals[i * 3 + 0];
                                mesh.normals[i * 3 + 1] = normals[i * 3 + 1];
                                mesh.normals[i * 3 + 2] = normals[i * 3 + 2];
                            }
                        }
                    }
                }
                else if (attr.first == "TANGENT")
                {
                    mesh.tangents.resize(accessor.count * 4);
                    if (accessor.type == TINYGLTF_TYPE_VEC4)
                    {
                        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                        {
                            const float* tangents = reinterpret_cast<const float*>(dataPtr);
                            for (size_t i = 0; i < accessor.count; ++i)
                            {
                                mesh.tangents[i * 4 + 0] = tangents[i * 4 + 0];
                                mesh.tangents[i * 4 + 1] = tangents[i * 4 + 1];
                                mesh.tangents[i * 4 + 2] = tangents[i * 4 + 2];
                                mesh.tangents[i * 4 + 3] = tangents[i * 4 + 3];
                            }
                        }
                    }
                }
                else if (attr.first == "TEXCOORD_0")
                {
                    mesh.texcoords0.resize(accessor.count * 2);
                    if (accessor.type == TINYGLTF_TYPE_VEC2)
                    {
                        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                        {
                            const float* texcoords = reinterpret_cast<const float*>(dataPtr);
                            for (size_t i = 0; i < accessor.count; ++i)
                            {
                                mesh.texcoords0[i * 2 + 0] = texcoords[i * 2 + 0];
                                mesh.texcoords0[i * 2 + 1] = texcoords[i * 2 + 1];
                            }
                        }
                    }
                }
                else if (attr.first == "TEXCOORD_1")
                {
                    mesh.texcoords1.resize(accessor.count * 2);
                    if (accessor.type == TINYGLTF_TYPE_VEC2)
                    {
                        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                        {
                            const float* texcoords = reinterpret_cast<const float*>(dataPtr);
                            for (size_t i = 0; i < accessor.count; ++i)
                            {
                                mesh.texcoords1[i * 2 + 0] = texcoords[i * 2 + 0];
                                mesh.texcoords1[i * 2 + 1] = texcoords[i * 2 + 1];
                            }
                        }
                    }
                }
                else if (attr.first == "COLOR_0")
                {
                    if (accessor.type == TINYGLTF_TYPE_VEC3)
                    {
                        mesh.colors.resize(accessor.count * 3);
                        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                        {
                            const float* colors = reinterpret_cast<const float*>(dataPtr);
                            for (size_t i = 0; i < accessor.count; ++i)
                            {
                                mesh.colors[i * 3 + 0] = colors[i * 3 + 0];
                                mesh.colors[i * 3 + 1] = colors[i * 3 + 1];
                                mesh.colors[i * 3 + 2] = colors[i * 3 + 2];
                            }
                        }
                    }
                    else if (accessor.type == TINYGLTF_TYPE_VEC4)
                    {
                        mesh.colors.resize(accessor.count * 4);
                        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                        {
                            const float* colors = reinterpret_cast<const float*>(dataPtr);
                            for (size_t i = 0; i < accessor.count; ++i)
                            {
                                mesh.colors[i * 4 + 0] = colors[i * 4 + 0];
                                mesh.colors[i * 4 + 1] = colors[i * 4 + 1];
                                mesh.colors[i * 4 + 2] = colors[i * 4 + 2];
                                mesh.colors[i * 4 + 3] = colors[i * 4 + 3];
                            }
                        }
                    }
                }
            }

            // Generate normals if needed and not present
            if (mesh.normals.empty() && info.generateNormals)
            {
                const size_t vertexCount = mesh.positions.size() / 3;
                mesh.normals.resize(vertexCount * 3);

                // Generate flat normals
                for (size_t i = 0; i < mesh.indices.size(); i += 3)
                {
                    const uint32_t i0 = mesh.indices[i + 0];
                    const uint32_t i1 = mesh.indices[i + 1];
                    const uint32_t i2 = mesh.indices[i + 2];

                    const float* v0 = &mesh.positions[i0 * 3];
                    const float* v1 = &mesh.positions[i1 * 3];
                    const float* v2 = &mesh.positions[i2 * 3];

                    // Calculate face normal
                    float nx = (v1[1] - v0[1]) * (v2[2] - v0[2]) - (v1[2] - v0[2]) * (v2[1] - v0[1]);
                    float ny = (v1[2] - v0[2]) * (v2[0] - v0[0]) - (v1[0] - v0[0]) * (v2[2] - v0[2]);
                    float nz = (v1[0] - v0[0]) * (v2[1] - v0[1]) - (v1[1] - v0[1]) * (v2[0] - v0[0]);

                    // Normalize
                    float len = std::sqrt(nx * nx + ny * ny + nz * nz);
                    if (len > 0.0f)
                    {
                        nx /= len;
                        ny /= len;
                        nz /= len;
                    }

                    // Add to all vertices
                    mesh.normals[i0 * 3 + 0] += nx;
                    mesh.normals[i0 * 3 + 1] += ny;
                    mesh.normals[i0 * 3 + 2] += nz;

                    mesh.normals[i1 * 3 + 0] += nx;
                    mesh.normals[i1 * 3 + 1] += ny;
                    mesh.normals[i1 * 3 + 2] += nz;

                    mesh.normals[i2 * 3 + 0] += nx;
                    mesh.normals[i2 * 3 + 1] += ny;
                    mesh.normals[i2 * 3 + 2] += nz;
                }

                // Normalize all normals
                for (size_t i = 0; i < vertexCount; ++i)
                {
                    float nx = mesh.normals[i * 3 + 0];
                    float ny = mesh.normals[i * 3 + 1];
                    float nz = mesh.normals[i * 3 + 2];

                    float len = std::sqrt(nx * nx + ny * ny + nz * nz);
                    if (len > 0.0f)
                    {
                        mesh.normals[i * 3 + 0] = nx / len;
                        mesh.normals[i * 3 + 1] = ny / len;
                        mesh.normals[i * 3 + 2] = nz / len;
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

        meshletBuilder.addMesh(
            mesh.positions.data(), 3 * sizeof(float), static_cast<uint32_t>(mesh.positions.size() / 3),
            mesh.indices.data(), static_cast<uint32_t>(mesh.indices.size()),
            mesh.normals.empty() ? nullptr : mesh.normals.data(), mesh.normals.empty() ? 0 : 3 * sizeof(float));
    }

    // Determine build flags from optimization flags
    bool optimizeOverdraw =
        (info.optimizationFlags & GeometryOptimizationBits::eOverdraw) != GeometryOptimizationBits::eNone;
    bool optimizeVertexFetch =
        (info.optimizationFlags & GeometryOptimizationBits::eVertexFetch) != GeometryOptimizationBits::eNone;

    // Build the meshlets
    meshletBuilder.build(info.maxVertsPerMeshlet, info.maxPrimsPerMeshlet, optimizeOverdraw, optimizeVertexFetch);

    // Extract the meshlet data
    std::vector<Meshlet> meshlets;
    std::vector<uint32_t> meshletVertices;
    std::vector<uint32_t> meshletIndices;

    meshletBuilder.exportMeshletData(meshlets, meshletVertices, meshletIndices);

    // Create submeshes (for now, just one submesh for all meshlets)
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
        float position[3];
        float normal[3];
        float texcoord[2];
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
        vertices.resize(baseVertex + vertexCount);

        for (size_t i = 0; i < vertexCount; ++i)
        {
            VertexData& v = vertices[baseVertex + i];

            // Position (always available)
            v.position[0] = mesh.positions[i * 3 + 0];
            v.position[1] = mesh.positions[i * 3 + 1];
            v.position[2] = mesh.positions[i * 3 + 2];

            // Normal (might be empty)
            if (i * 3 + 2 < mesh.normals.size())
            {
                v.normal[0] = mesh.normals[i * 3 + 0];
                v.normal[1] = mesh.normals[i * 3 + 1];
                v.normal[2] = mesh.normals[i * 3 + 2];
            }
            else
            {
                v.normal[0] = 0.0f;
                v.normal[1] = 1.0f;
                v.normal[2] = 0.0f;
            }

            // Texcoord (might be empty)
            if (i * 2 + 1 < mesh.texcoords0.size())
            {
                v.texcoord[0] = mesh.texcoords0[i * 2 + 0];
                v.texcoord[1] = mesh.texcoords0[i * 2 + 1];
            }
            else
            {
                v.texcoord[0] = 0.0f;
                v.texcoord[1] = 0.0f;
            }
        }

        // Create indices (adjust for base vertex offset)
        const size_t baseIndex = indices.size();
        indices.resize(baseIndex + mesh.indices.size());

        for (size_t i = 0; i < mesh.indices.size(); ++i)
        {
            indices[baseIndex + i] = static_cast<uint32_t>(baseVertex) + mesh.indices[i];
        }
    }

    // Create separate position and attribute buffers for better cache behavior
    std::vector<float> positions;
    positions.reserve(vertices.size() * 3);

    std::vector<float> attributes;
    attributes.reserve(vertices.size() * 5); // 3 for normal + 2 for texcoord

    for (const auto& v : vertices)
    {
        positions.push_back(v.position[0]);
        positions.push_back(v.position[1]);
        positions.push_back(v.position[2]);

        attributes.push_back(v.normal[0]);
        attributes.push_back(v.normal[1]);
        attributes.push_back(v.normal[2]);

        attributes.push_back(v.texcoord[0]);
        attributes.push_back(v.texcoord[1]);
    }

    // Create GPU data structure
    GeometryGpuData gpuData;
    gpuData.vertexCount             = static_cast<uint32_t>(vertices.size());
    gpuData.indexCount              = static_cast<uint32_t>(indices.size());
    gpuData.meshletCount            = static_cast<uint32_t>(meshlets.size());
    gpuData.meshletMaxVertexCount   = info.maxVertsPerMeshlet;
    gpuData.meshletMaxTriangleCount = info.maxPrimsPerMeshlet;

    // Determine index type based on vertex count
    gpuData.indexType = (vertices.size() > UINT16_MAX) ? IndexType::UINT32 : IndexType::UINT16;

    // Create position buffer
    {
        BufferLoadInfo bufferInfo{
            .debugName   = info.debugName + "::position_buffer",
            .data        = positions.data(),
            .dataSize    = positions.size() * sizeof(float),
            .createInfo  = { .size   = static_cast<uint32_t>(positions.size() * sizeof(float)),
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
            .data        = attributes.data(),
            .dataSize    = attributes.size() * sizeof(float),
            .createInfo  = { .size   = static_cast<uint32_t>(attributes.size() * sizeof(float)),
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
            std::vector<uint16_t> indices16(indices.size());
            for (size_t i = 0; i < indices.size(); ++i)
            {
                indices16[i] = static_cast<uint16_t>(indices[i]);
            }

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
