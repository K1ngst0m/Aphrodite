#include "shaderReflector.h"
#include "reflectionSerialization.h"

#include "common/profiler.h"

#include "api/vulkan/sampler.h"
#include "spirv_cross.hpp"

namespace aph
{
namespace
{
// Map from SPIR-V base types to their sizes in bytes
const HashMap<spirv_cross::SPIRType::BaseType, size_t> baseTypeSizeMap = {
    { spirv_cross::SPIRType::Float, 4},
    {   spirv_cross::SPIRType::Int, 4},
    {  spirv_cross::SPIRType::UInt, 4},
    {spirv_cross::SPIRType::Double, 8},
    // TODO: Add other base types as needed
};

/**
 * Calculate the size of a SPIR-V type in bytes
 */
std::size_t getTypeSize(const spirv_cross::SPIRType& type)
{
    APH_ASSERT(baseTypeSizeMap.contains(type.basetype));
    size_t baseSize = baseTypeSizeMap.at(type.basetype);

    // Calculate size for vectors and matrices
    size_t elementCount = type.vecsize * type.columns;
    size_t size         = baseSize * elementCount;

    // Handle arrays
    if (!type.array.empty())
    {
        size_t arraySize = 1;
        for (size_t length : type.array)
            arraySize *= length;
        size *= arraySize;
    }

    return size;
}

/**
 * Convert a SPIR-V type to a Vulkan format
 */
VkFormat spirTypeToVkFormat(const spirv_cross::SPIRType& type)
{
    // Handle scalars (vecsize = 1)
    if (type.vecsize == 1)
    {
        switch (type.basetype)
        {
        case spirv_cross::SPIRType::Float:
            return VK_FORMAT_R32_SFLOAT;
        case spirv_cross::SPIRType::Int:
            return VK_FORMAT_R32_SINT;
        case spirv_cross::SPIRType::UInt:
            return VK_FORMAT_R32_UINT;
        default:
            APH_ASSERT(false);
            return VK_FORMAT_UNDEFINED;
        }
    }
    // Handle 2-component vectors
    else if (type.vecsize == 2)
    {
        switch (type.basetype)
        {
        case spirv_cross::SPIRType::Float:
            return VK_FORMAT_R32G32_SFLOAT;
        case spirv_cross::SPIRType::Int:
            return VK_FORMAT_R32G32_SINT;
        case spirv_cross::SPIRType::UInt:
            return VK_FORMAT_R32G32_UINT;
        default:
            APH_ASSERT(false);
            return VK_FORMAT_UNDEFINED;
        }
    }
    // Handle 3-component vectors
    else if (type.vecsize == 3)
    {
        switch (type.basetype)
        {
        case spirv_cross::SPIRType::Float:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case spirv_cross::SPIRType::Int:
            return VK_FORMAT_R32G32B32_SINT;
        case spirv_cross::SPIRType::UInt:
            return VK_FORMAT_R32G32B32_UINT;
        default:
            APH_ASSERT(false);
            return VK_FORMAT_UNDEFINED;
        }
    }
    // Handle 4-component vectors
    else if (type.vecsize == 4)
    {
        switch (type.basetype)
        {
        case spirv_cross::SPIRType::Float:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case spirv_cross::SPIRType::Int:
            return VK_FORMAT_R32G32B32A32_SINT;
        case spirv_cross::SPIRType::UInt:
            return VK_FORMAT_R32G32B32A32_UINT;
        default:
            APH_ASSERT(false);
            return VK_FORMAT_UNDEFINED;
        }
    }

    // Fallback for unsupported types
    return VK_FORMAT_UNDEFINED;
}

/**
 * Update the array size information for a resource binding
 */
void updateArrayInfo(ResourceLayout& resourceLayout, const spirv_cross::SPIRType& type, unsigned set, unsigned binding)
{
    auto& size = resourceLayout.layouts[set].arraySize[binding];
    if (!type.array.empty())
    {
        if (type.array.size() != 1)
        {
            VK_LOG_ERR("Array dimension must be 1.");
            return;
        }

        if (!type.array_size_literal.front())
        {
            VK_LOG_ERR("Array dimension must be a literal.");
            return;
        }

        if (type.array.front() == 0)
        {
            // Mark as bindless
            resourceLayout.bindlessSetMask.set(set);
            resourceLayout.layouts[set].fpMask.reset();
            size = ShaderLayout::UNSIZED_ARRAY;
        }
        else if (size && size != type.array.front())
        {
            VK_LOG_ERR("Array dimension for (%u, %u) is inconsistent.", set, binding);
        }
        else if (type.array.front() + binding > VULKAN_NUM_BINDINGS)
        {
            VK_LOG_ERR("Binding array will go out of bounds.");
        }
        else
        {
            size = uint8_t(type.array.front());
        }
    }
    else
    {
        if (size && size != 1)
        {
            VK_LOG_ERR("Array dimension for (%u, %u) is inconsistent.", set, binding);
        }
        size = 1;
    }
}
} // anonymous namespace

/**
 * Private implementation class for ShaderReflector
 * This class contains all the SPIRV-Cross dependent code
 */
class ShaderReflector::Impl
{
public:
    Impl() = default;

    /**
     * Performs reflection on a set of shaders
     */
    ReflectionResult reflectShaders(const ReflectRequest& request)
    {
        APH_PROFILER_SCOPE();

        // Store the request for later use
        m_request = request;

        // Create a new result object
        ReflectionResult result;

        // Perform the reflection process
        reflect();

        // Build VkDescriptorSetLayoutBindings and pool sizes
        createDescriptorSetInfo();

        // Fill in the result structure
        result.vertexInput       = m_vertexInput;
        result.resourceLayout    = m_combinedLayout;
        result.pushConstantRange = m_combinedLayout.pushConstantRange;

        // Copy descriptor set information
        for (uint32_t set = 0; set < VULKAN_NUM_DESCRIPTOR_SETS; set++)
        {
            result.descriptorResources[set].bindings  = m_setInfos[set].bindings;
            result.descriptorResources[set].poolSizes = m_setInfos[set].poolSizes;
        }

        return result;
    }

private:
    /**
     * Performs the reflection process across all shaders
     */
    void reflect()
    {
        APH_PROFILER_SCOPE();
        const auto& shaders = m_request.shaders;
        const auto& options = m_request.options;

        // Reflect each shader stage
        for (const auto& shader : shaders)
        {
            APH_ASSERT(shader);
            const auto& stage        = shader->getStage();
            const auto& shaderLayout = reflectStageLayout(shader->getCode(), options);
            m_stageLayouts[stage]    = shaderLayout;

            // Handle stage-specific data (VS attributes, FS targets)
            if (stage == ShaderStage::VS && options.extractInputAttributes)
            {
                m_combinedLayout.attributeMask = shaderLayout.inputMask;
                for (auto idx = 0; idx < VULKAN_NUM_VERTEX_ATTRIBS; ++idx)
                {
                    m_combinedLayout.vertexAttr[idx] = shaderLayout.vertexAttributes[idx];
                }
            }
            else if (stage == ShaderStage::FS && options.extractOutputAttributes)
            {
                m_combinedLayout.renderTargetMask = shaderLayout.outputMask;
            }

            // Combine layout information across all shader stages
            combineLayouts(stage, shaderLayout);
        }

        // Handle immutable samplers and validate bindings
        if (m_request.options.validateBindings)
        {
            processSets();
        }
    }

    /**
     * Creates descriptor set layout bindings and pool sizes from reflection data
     */
    void createDescriptorSetInfo()
    {
        APH_PROFILER_SCOPE();

        auto& combinedSetInfos = m_combinedLayout.setInfos;

        // Extract vertex input data if present in vertex shader
        if (m_stageLayouts.contains(ShaderStage::VS))
        {
            extractVertexInputData();
        }

        // Process each descriptor set
        for (unsigned set = 0; set < VULKAN_NUM_DESCRIPTOR_SETS; set++)
        {
            CombinedResourceLayout::SetInfo& setInfo = combinedSetInfos[set];
            const auto& shaderLayout                 = setInfo.shaderLayout;
            const auto& stageForBinds                = setInfo.stagesForBindings;

            SmallVector<::vk::DescriptorSetLayoutBinding>& vkBindings = m_setInfos[set].bindings;
            SmallVector<::vk::DescriptorPoolSize>& poolSizes          = m_setInfos[set].poolSizes;

            bool hasBindless = false;

            // Process each binding
            for (unsigned binding = 0; binding < VULKAN_NUM_BINDINGS; binding++)
            {
                const auto stages = vk::utils::VkCast(stageForBinds[binding]);
                if (!stages)
                {
                    continue; // Skip unused bindings
                }

                // Handle array sizes
                unsigned arraySize     = shaderLayout.arraySize[binding];
                unsigned poolArraySize = 0;

                if (arraySize == ShaderLayout::UNSIZED_ARRAY)
                {
                    arraySize     = VULKAN_NUM_BINDINGS_BINDLESS_VARYING;
                    poolArraySize = arraySize;
                    hasBindless   = true;
                }
                else
                {
                    poolArraySize = arraySize * VULKAN_NUM_SETS_PER_POOL;
                }

                // Add bindings and pool sizes for each resource type
                addResourceBindings(vkBindings, poolSizes, shaderLayout, binding, arraySize, poolArraySize, stages,
                                    ::vk::Sampler{}, hasBindless);
            }

            // Set full shader stages visibility for all bindings
            for (auto& vkBinding : vkBindings)
            {
                vkBinding.setStageFlags(::vk::ShaderStageFlagBits::eAll);
            }
        }
    }

    void extractVertexInputData()
    {
        const auto& shaderLayout = m_stageLayouts[ShaderStage::VS];
        VertexInput& vertexInput = m_vertexInput;
        uint32_t size            = 0;

        // Process input attributes
        for (uint32_t location : aph::utils::forEachBit(shaderLayout.inputMask))
        {
            const auto& attr = shaderLayout.vertexAttributes[location];
            vertexInput.attributes.push_back(
                {.location = location, .binding = attr.binding, .format = attr.format, .offset = attr.offset});
            size += attr.size;
        }

        // Set up vertex bindings
        vertexInput.bindings.push_back({size});
    }

    void addResourceBindings(SmallVector<::vk::DescriptorSetLayoutBinding>& bindings,
                             SmallVector<::vk::DescriptorPoolSize>& poolSizes, const ShaderLayout& layout,
                             unsigned binding, unsigned arraySize, unsigned poolArraySize,
                             ::vk::ShaderStageFlags stages, ::vk::Sampler, bool hasBindless)
    {
        unsigned types = 0;

        // Combined image samplers
        if (layout.sampledImageMask.test(binding))
        {
            bindings.push_back({binding, ::vk::DescriptorType::eCombinedImageSampler, arraySize, stages, nullptr});
            poolSizes.push_back({::vk::DescriptorType::eCombinedImageSampler, poolArraySize});
            types++;
        }

        // Uniform texel buffers
        if (layout.sampledTexelBufferMask.test(binding))
        {
            bindings.push_back({binding, ::vk::DescriptorType::eUniformTexelBuffer, arraySize, stages, nullptr});
            poolSizes.push_back({::vk::DescriptorType::eUniformTexelBuffer, poolArraySize});
            types++;
        }

        // Storage texel buffers
        if (layout.storageTexelBufferMask.test(binding))
        {
            bindings.push_back({binding, ::vk::DescriptorType::eStorageTexelBuffer, arraySize, stages, nullptr});
            poolSizes.push_back({::vk::DescriptorType::eStorageTexelBuffer, poolArraySize});
            types++;
        }

        // Storage images
        if (layout.storageImageMask.test(binding))
        {
            bindings.push_back({binding, ::vk::DescriptorType::eStorageImage, arraySize, stages, nullptr});
            poolSizes.push_back({::vk::DescriptorType::eStorageImage, poolArraySize});
            types++;
        }

        // Uniform buffers
        if (layout.uniformBufferMask.test(binding))
        {
            auto descriptorType =
                hasBindless ? ::vk::DescriptorType::eUniformBuffer : ::vk::DescriptorType::eUniformBufferDynamic;
            bindings.push_back({binding, descriptorType, arraySize, stages, nullptr});
            poolSizes.push_back({descriptorType, poolArraySize});
            types++;
        }

        // Storage buffers
        if (layout.storageBufferMask.test(binding))
        {
            bindings.push_back({binding, ::vk::DescriptorType::eStorageBuffer, arraySize, stages, nullptr});
            poolSizes.push_back({::vk::DescriptorType::eStorageBuffer, poolArraySize});
            types++;
        }

        // Input attachments
        if (layout.inputAttachmentMask.test(binding))
        {
            bindings.push_back({binding, ::vk::DescriptorType::eInputAttachment, arraySize, stages, nullptr});
            poolSizes.push_back({::vk::DescriptorType::eInputAttachment, poolArraySize});
            types++;
        }

        // Separate images
        if (layout.separateImageMask.test(binding))
        {
            bindings.push_back({binding, ::vk::DescriptorType::eSampledImage, arraySize, stages, nullptr});
            poolSizes.push_back({::vk::DescriptorType::eSampledImage, poolArraySize});
            types++;
        }

        // Samplers
        if (layout.samplerMask.test(binding))
        {
            bindings.push_back({binding, ::vk::DescriptorType::eSampler, arraySize, stages, nullptr});
            poolSizes.push_back({::vk::DescriptorType::eSampler, poolArraySize});
            types++;
        }

        // Ensure no aliasing of descriptor bindings
        APH_ASSERT(types <= 1 && "Descriptor set aliasing!");
    }

    ResourceLayout reflectStageLayout(ArrayProxy<uint32_t> spvCode, const ReflectionOptions& options)
    {
        APH_PROFILER_SCOPE();
        spirv_cross::Compiler compiler{spvCode.data(), spvCode.size()};
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();

        ResourceLayout layout{};

        // Process stage inputs (vertex attributes)
        if (options.extractInputAttributes)
        {
            reflectStageInputs(compiler, resources, layout);
        }

        // Process stage outputs
        if (options.extractOutputAttributes)
        {
            reflectStageOutputs(compiler, resources, layout);
        }

        // Process uniform buffers
        reflectUniformBuffers(compiler, resources, layout);

        // Process storage buffers
        reflectStorageBuffers(compiler, resources, layout);

        // Process storage images
        reflectStorageImages(compiler, resources, layout);

        // Process sampled images
        reflectSampledImages(compiler, resources, layout);

        // Process separate images
        reflectSeparateImages(compiler, resources, layout);

        // Process separate samplers
        reflectSeparateSamplers(compiler, resources, layout);

        // Process push constants
        if (options.extractPushConstants && !resources.push_constant_buffers.empty())
        {
            layout.pushConstantSize = compiler.get_declared_struct_size(
                compiler.get_type(resources.push_constant_buffers.front().base_type_id));
        }

        // Process specialization constants
        if (options.extractSpecConstants)
        {
            reflectSpecConstants(compiler, layout);
        }

        return layout;
    }

    void reflectStageInputs(spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources,
                            ResourceLayout& layout)
    {
        for (const auto& res : resources.stage_inputs)
        {
            uint32_t location = compiler.get_decoration(res.id, spv::DecorationLocation);
            layout.inputMask.set(location);
            APH_ASSERT(layout.inputMask.size() > location);
            const spirv_cross::SPIRType& type = compiler.get_type(res.type_id);

            VkFormat format = spirTypeToVkFormat(type);

            layout.vertexAttributes[location] = {
                // TODO multiple bindings
                .binding = 0,
                .format  = vk::utils::getFormatFromVk(format),
                .size    = static_cast<uint32_t>(getTypeSize(type)),
            };
        }

        // Calculate offsets for vertex attributes
        uint32_t attrOffset = 0;
        for (uint32_t location : aph::utils::forEachBit(layout.inputMask))
        {
            auto& attr  = layout.vertexAttributes[location];
            attr.offset = attrOffset;
            attrOffset += attr.size;
        }
    }

    void reflectStageOutputs(spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources,
                             ResourceLayout& layout)
    {
        for (const auto& res : resources.stage_outputs)
        {
            auto location = compiler.get_decoration(res.id, spv::DecorationLocation);
            layout.outputMask.set(location);
        }
    }

    void reflectUniformBuffers(spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources,
                               ResourceLayout& layout)
    {
        for (const auto& res : resources.uniform_buffers)
        {
            uint32_t set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);
            APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
            APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

            layout.layouts[set].uniformBufferMask.set(binding);
            updateArrayInfo(layout, compiler.get_type(res.type_id), set, binding);
        }
    }

    void reflectStorageBuffers(spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources,
                               ResourceLayout& layout)
    {
        for (const auto& res : resources.storage_buffers)
        {
            unsigned set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
            unsigned binding = compiler.get_decoration(res.id, spv::DecorationBinding);
            APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
            APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

            layout.layouts[set].storageBufferMask.set(binding);
            updateArrayInfo(layout, compiler.get_type(res.type_id), set, binding);
        }
    }

    void reflectStorageImages(spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources,
                              ResourceLayout& layout)
    {
        for (const auto& res : resources.storage_images)
        {
            unsigned set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
            unsigned binding = compiler.get_decoration(res.id, spv::DecorationBinding);
            APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
            APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

            auto& type = compiler.get_type(res.type_id);
            if (type.image.dim == spv::DimBuffer)
            {
                layout.layouts[set].storageTexelBufferMask.set(binding);
            }
            else
            {
                layout.layouts[set].storageImageMask.set(binding);
            }

            if (compiler.get_type(type.image.type).basetype == spirv_cross::SPIRType::BaseType::Float)
            {
                layout.layouts[set].fpMask.set(binding);
            }

            updateArrayInfo(layout, type, set, binding);
        }
    }

    void reflectSampledImages(spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources,
                              ResourceLayout& layout)
    {
        for (const auto& res : resources.sampled_images)
        {
            unsigned set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
            unsigned binding = compiler.get_decoration(res.id, spv::DecorationBinding);
            APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
            APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

            auto& type = compiler.get_type(res.type_id);
            if (compiler.get_type(type.image.type).basetype == spirv_cross::SPIRType::BaseType::Float)
            {
                layout.layouts[set].fpMask.set(binding);
            }
            if (type.image.dim == spv::DimBuffer)
            {
                layout.layouts[set].sampledTexelBufferMask.set(binding);
            }
            else
            {
                layout.layouts[set].sampledImageMask.set(binding);
            }
            updateArrayInfo(layout, type, set, binding);
        }
    }

    void reflectSeparateImages(spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources,
                               ResourceLayout& layout)
    {
        for (const auto& res : resources.separate_images)
        {
            unsigned set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
            unsigned binding = compiler.get_decoration(res.id, spv::DecorationBinding);
            APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
            APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

            auto& type = compiler.get_type(res.type_id);
            if (compiler.get_type(type.image.type).basetype == spirv_cross::SPIRType::BaseType::Float)
            {
                layout.layouts[set].fpMask.set(binding);
            }
            if (type.image.dim == spv::DimBuffer)
            {
                layout.layouts[set].sampledTexelBufferMask.set(binding);
            }
            else
            {
                layout.layouts[set].separateImageMask.set(binding);
            }
            updateArrayInfo(layout, type, set, binding);
        }
    }

    void reflectSeparateSamplers(spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources,
                                 ResourceLayout& layout)
    {
        for (const auto& res : resources.separate_samplers)
        {
            unsigned set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
            unsigned binding = compiler.get_decoration(res.id, spv::DecorationBinding);
            APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
            APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

            layout.layouts[set].samplerMask.set(binding);
            updateArrayInfo(layout, compiler.get_type(res.type_id), set, binding);
        }
    }

    void reflectSpecConstants(spirv_cross::Compiler& compiler, ResourceLayout& layout)
    {
        auto specConstants = compiler.get_specialization_constants();
        for (auto& c : specConstants)
        {
            if (c.constant_id >= VULKAN_NUM_TOTAL_SPEC_CONSTANTS)
            {
                VK_LOG_ERR("Spec constant ID: %u is out of range, will be ignored.", c.constant_id);
                continue;
            }

            layout.specConstantMask.set(c.constant_id);
        }
    }

    void combineLayouts(ShaderStage stage, const ResourceLayout& shaderLayout)
    {
        auto& combinedSetInfos = m_combinedLayout.setInfos;

        // Combine layouts for all descriptor sets
        for (unsigned i = 0; i < VULKAN_NUM_DESCRIPTOR_SETS; i++)
        {
            CombinedResourceLayout::SetInfo& combinedSetInfo = combinedSetInfos[i];

            // Combine resource masks for this set
            combinedSetInfo.shaderLayout.sampledImageMask |= shaderLayout.layouts[i].sampledImageMask;
            combinedSetInfo.shaderLayout.storageImageMask |= shaderLayout.layouts[i].storageImageMask;
            combinedSetInfo.shaderLayout.uniformBufferMask |= shaderLayout.layouts[i].uniformBufferMask;
            combinedSetInfo.shaderLayout.storageBufferMask |= shaderLayout.layouts[i].storageBufferMask;
            combinedSetInfo.shaderLayout.sampledTexelBufferMask |= shaderLayout.layouts[i].sampledTexelBufferMask;
            combinedSetInfo.shaderLayout.storageTexelBufferMask |= shaderLayout.layouts[i].storageTexelBufferMask;
            combinedSetInfo.shaderLayout.inputAttachmentMask |= shaderLayout.layouts[i].inputAttachmentMask;
            combinedSetInfo.shaderLayout.samplerMask |= shaderLayout.layouts[i].samplerMask;
            combinedSetInfo.shaderLayout.separateImageMask |= shaderLayout.layouts[i].separateImageMask;
            combinedSetInfo.shaderLayout.fpMask |= shaderLayout.layouts[i].fpMask;

            // Calculate which bindings are active across all resource types
            auto activeBinds = shaderLayout.layouts[i].sampledImageMask | shaderLayout.layouts[i].storageImageMask |
                               shaderLayout.layouts[i].uniformBufferMask | shaderLayout.layouts[i].storageBufferMask |
                               shaderLayout.layouts[i].sampledTexelBufferMask |
                               shaderLayout.layouts[i].storageTexelBufferMask |
                               shaderLayout.layouts[i].inputAttachmentMask | shaderLayout.layouts[i].samplerMask |
                               shaderLayout.layouts[i].separateImageMask;

            // If this set has any active bindings, mark the set as used by this stage
            if (activeBinds.any())
            {
                combinedSetInfo.stagesForSets |= stage;
            }

            // For each active binding, mark which stages use it and validate array sizes
            for (uint32_t bit : aph::utils::forEachBit(activeBinds))
            {
                combinedSetInfo.stagesForBindings[bit] |= stage;

                auto& combinedSize = combinedSetInfo.shaderLayout.arraySize[bit];
                auto& shaderSize   = shaderLayout.layouts[i].arraySize[bit];
                if (combinedSize && combinedSize != shaderSize)
                {
                    VK_LOG_ERR("Mismatch between array sizes in different shaders.");
                    APH_ASSERT(false);
                }
                else
                {
                    combinedSize = shaderSize;
                }
            }
        }

        // Merge push constant ranges
        if (shaderLayout.pushConstantSize != 0)
        {
            m_combinedLayout.pushConstantRange.stageFlags |= stage;
            m_combinedLayout.pushConstantRange.size =
                std::max(m_combinedLayout.pushConstantRange.size, shaderLayout.pushConstantSize);
        }

        // Combine specialization constants
        m_combinedLayout.specConstantMask[stage] = shaderLayout.specConstantMask;
        m_combinedLayout.combinedSpecConstantMask |= shaderLayout.specConstantMask;
        m_combinedLayout.bindlessDescriptorSetMask |= shaderLayout.bindlessSetMask;
    }

    void processSets()
    {
        auto& combinedSetInfos = m_combinedLayout.setInfos;

        // Process each descriptor set
        for (unsigned setIdx = 0; setIdx < VULKAN_NUM_DESCRIPTOR_SETS; setIdx++)
        {
            CombinedResourceLayout::SetInfo& setInfo = combinedSetInfos[setIdx];

            // Skip empty sets
            if (!setInfo.stagesForSets)
            {
                continue;
            }

            // Mark this set as used
            m_combinedLayout.descriptorSetMask.set(setIdx);

            // Process bindings in this set
            for (unsigned binding = 0; binding < VULKAN_NUM_BINDINGS; binding++)
            {
                auto& arraySize = setInfo.shaderLayout.arraySize[binding];
                if (arraySize == ShaderLayout::UNSIZED_ARRAY)
                {
                    // For bindless resources, make visible to all shader stages
                    setInfo.stagesForBindings[binding] = ShaderStage::All;
                }
                else if (arraySize == 0)
                {
                    // Default to array size 1 for non-array resources
                    arraySize = 1;
                }
                else
                {
                    // Check for binding conflicts with arrays
                    for (unsigned i = 1; i < arraySize; i++)
                    {
                        if (setInfo.stagesForBindings[binding + i])
                        {
                            VK_LOG_ERR(
                                "Detected binding aliasing for (%u, %u). Binding array with %u elements starting "
                                "at (%u, "
                                "%u) overlaps.\n",
                                setIdx, binding + i, arraySize, setIdx, binding);
                        }
                    }
                }
            }
        }
    }

    // Member variables
    ReflectRequest m_request;
    VertexInput m_vertexInput;
    CombinedResourceLayout m_combinedLayout;
    HashMap<ShaderStage, ResourceLayout> m_stageLayouts;

    // Internal structure for descriptor set information
    struct SetInfo
    {
        SmallVector<::vk::DescriptorSetLayoutBinding> bindings;
        SmallVector<::vk::DescriptorPoolSize> poolSizes;
    };
    SetInfo m_setInfos[VULKAN_NUM_DESCRIPTOR_SETS] = {};
};

// ShaderReflector implementation that delegates to the private implementation
ShaderReflector::ShaderReflector()
    : m_impl(std::make_unique<Impl>())
{
}

ShaderReflector::~ShaderReflector() = default;

ReflectionResult ShaderReflector::reflect(const ReflectRequest& request)
{
    // Check if we should try to load from cache first
    if (request.options.enableCaching && !request.options.cachePath.empty() && !request.options.forceUncached)
    {
        VK_LOG_INFO("Looking for shader reflection cache at: %s", request.options.cachePath.c_str());

        // Create cache path if needed (using std::filesystem for path operations)
        std::filesystem::path cachePath(request.options.cachePath);

        // Check if cache file exists
        if (std::filesystem::exists(cachePath))
        {
            VK_LOG_INFO("Found shader reflection cache, loading");

            // Load reflection data from cache
            ReflectionResult cachedResult;
            Result loadResult = reflection::loadReflectionFromFile(cachePath, cachedResult);
            if (loadResult)
            {
                VK_LOG_INFO("Successfully loaded shader reflection from cache");
                return cachedResult;
            }
            else
            {
                VK_LOG_WARN("Failed to load shader reflection cache: %s", loadResult.toString());
            }
        }
        else
        {
            VK_LOG_INFO("No shader reflection cache found at: %s", request.options.cachePath.c_str());
        }
    }
    else if (request.options.forceUncached)
    {
        VK_LOG_INFO("Skipping shader reflection cache due to forceUncached flag");
    }

    // If we couldn't load from cache, proceed with normal reflection
    VK_LOG_INFO("Performing shader reflection");

    ReflectionResult result = m_impl->reflectShaders(request);

    // If caching is enabled, save the reflection results
    if (request.options.enableCaching && !request.options.cachePath.empty() && !request.options.forceUncached)
    {
        VK_LOG_INFO("Saving shader reflection cache to: %s", request.options.cachePath.c_str());

        if (reflection::saveReflectionToFile(result, request.options.cachePath))
        {
            VK_LOG_INFO("Successfully saved shader reflection cache");
        }
        else
        {
            VK_LOG_WARN("Failed to save shader reflection cache");
        }
    }
    else if (request.options.forceUncached)
    {
        VK_LOG_INFO("Skipping shader reflection cache creation due to forceUncached flag");
    }

    return result;
}

SmallVector<::vk::DescriptorSetLayoutBinding> ShaderReflector::getLayoutBindings(const ReflectionResult& result,
                                                                                 uint32_t set)
{
    APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
    return result.descriptorResources[set].bindings;
}

SmallVector<::vk::DescriptorPoolSize> ShaderReflector::getPoolSizes(const ReflectionResult& result, uint32_t set)
{
    APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
    return result.descriptorResources[set].poolSizes;
}

bool ShaderReflector::isBindlessSet(const ReflectionResult& result, uint32_t set)
{
    APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
    return result.resourceLayout.bindlessDescriptorSetMask.test(set);
}

SmallVector<uint32_t> ShaderReflector::getActiveDescriptorSets(const ReflectionResult& result)
{
    SmallVector<uint32_t> activeSets;
    for (uint32_t setIndex : aph::utils::forEachBit(result.resourceLayout.descriptorSetMask))
    {
        activeSets.push_back(setIndex);
    }
    return activeSets;
}

Result reflectShaders(const std::vector<spirv_cross::SmallVector<uint32_t>>& spvDatas,
                      const std::vector<spirv_cross::SmallVector<uint32_t>>& compDatas, ReflectionResult& outResult,
                      const ReflectionOptions& options)
{
    APH_PROFILER_SCOPE();

    // Check if we should try to load from cache first
    if (options.enableCaching && !options.cachePath.empty() && !options.forceUncached)
    {
        VK_LOG_INFO("Looking for shader reflection cache at: %s", options.cachePath.c_str());

        // Create cache path if needed (using std::filesystem for path operations)
        std::filesystem::path cachePath(options.cachePath);

        // Check if cache file exists
        if (std::filesystem::exists(cachePath))
        {
            VK_LOG_INFO("Found shader reflection cache, loading");

            // Load reflection data from cache
            Result loadResult = reflection::loadReflectionFromFile(cachePath, outResult);
            if (loadResult)
            {
                VK_LOG_INFO("Successfully loaded shader reflection from cache");
                return Result::Success;
            }
            else
            {
                VK_LOG_WARN("Failed to load shader reflection cache: %s", loadResult.toString());
            }
        }
        else
        {
            VK_LOG_INFO("No shader reflection cache found at: %s", options.cachePath.c_str());
        }
    }
    else if (options.forceUncached)
    {
        VK_LOG_INFO("Skipping shader reflection cache due to forceUncached flag");
    }

    // If we couldn't load from cache, proceed with normal reflection
    VK_LOG_INFO("Performing shader reflection");

    // ... existing reflection code ...

    // If caching is enabled, save the reflection results
    if (options.enableCaching && !options.cachePath.empty() && !options.forceUncached)
    {
        VK_LOG_INFO("Saving shader reflection cache to: %s", options.cachePath.c_str());

        if (reflection::saveReflectionToFile(outResult, options.cachePath))
        {
            VK_LOG_INFO("Successfully saved shader reflection cache");
        }
        else
        {
            VK_LOG_WARN("Failed to save shader reflection cache");
        }
    }
    else if (options.forceUncached)
    {
        VK_LOG_INFO("Skipping shader reflection cache creation due to forceUncached flag");
    }

    return Result::Success;
}

} // namespace aph
