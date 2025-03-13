#include "shaderReflector.h"
#include "api/vulkan/sampler.h"
#include "spirv_cross.hpp"

namespace aph
{
const std::unordered_map<spirv_cross::SPIRType::BaseType, size_t> baseTypeSizeMap = {
    { spirv_cross::SPIRType::Float, 4 },
    { spirv_cross::SPIRType::Int, 4 },
    { spirv_cross::SPIRType::UInt, 4 },
    { spirv_cross::SPIRType::Double, 8 },
    // TODO Add other base types as needed
};

std::size_t getTypeSize(const spirv_cross::SPIRType& type)
{
    // Lookup base size from the map
    APH_ASSERT(baseTypeSizeMap.contains(type.basetype));
    size_t baseSize = baseTypeSizeMap.at(type.basetype);

    // Calculate size for vectors and matrices
    size_t elementCount = type.vecsize * type.columns;
    size_t size = baseSize * elementCount;

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

VkFormat spirTypeToVkFormat(const spirv_cross::SPIRType& type)
{
    // Handle vector types
    if (type.vecsize == 1)
    { // Scalars
        switch (type.basetype)
        {
        case spirv_cross::SPIRType::Float:
            return VK_FORMAT_R32_SFLOAT;
        case spirv_cross::SPIRType::Int:
            return VK_FORMAT_R32_SINT;
        case spirv_cross::SPIRType::UInt:
            return VK_FORMAT_R32_UINT;
            // TODO Add other scalar types as needed
        default:
            APH_ASSERT(false);
            return VK_FORMAT_UNDEFINED;
        }
    }
    else if (type.vecsize == 2)
    { // 2-component vectors
        switch (type.basetype)
        {
        case spirv_cross::SPIRType::Float:
            return VK_FORMAT_R32G32_SFLOAT;
        case spirv_cross::SPIRType::Int:
            return VK_FORMAT_R32G32_SINT;
        case spirv_cross::SPIRType::UInt:
            return VK_FORMAT_R32G32_UINT;
            // TODO Add other 2-component types as needed
        default:
            APH_ASSERT(false);
            return VK_FORMAT_UNDEFINED;
        }
    }
    else if (type.vecsize == 3)
    { // 3-component vectors
        switch (type.basetype)
        {
        case spirv_cross::SPIRType::Float:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case spirv_cross::SPIRType::Int:
            return VK_FORMAT_R32G32B32_SINT;
        case spirv_cross::SPIRType::UInt:
            return VK_FORMAT_R32G32B32_UINT;
            // TODO Add other 3-component types as needed
        default:
            APH_ASSERT(false);
            return VK_FORMAT_UNDEFINED;
        }
    }
    else if (type.vecsize == 4)
    { // 4-component vectors
        switch (type.basetype)
        {
        case spirv_cross::SPIRType::Float:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case spirv_cross::SPIRType::Int:
            return VK_FORMAT_R32G32B32A32_SINT;
        case spirv_cross::SPIRType::UInt:
            return VK_FORMAT_R32G32B32A32_UINT;
            // TODO Add other 4-component types as needed
        default:
            APH_ASSERT(false);
            return VK_FORMAT_UNDEFINED;
        }
    }

    // Fallback or unsupported type
    return VK_FORMAT_UNDEFINED;
}

void updateArrayInfo(ResourceLayout& resourceLayout, const spirv_cross::SPIRType& type, unsigned set, unsigned binding)
{
    auto& size = resourceLayout.layouts[set].arraySize[binding];
    if (!type.array.empty())
    {
        if (type.array.size() != 1)
        {
            VK_LOG_ERR("Array dimension must be 1.");
        }
        else if (!type.array_size_literal.front())
        {
            VK_LOG_ERR("Array dimension must be a literal.");
        }
        else
        {
            if (type.array.front() == 0)
            {
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
    }
    else
    {
        if (size && size != 1)
        {
            VK_LOG_ERR("Array dimension for (%u, %u) is inconsistent.", set, binding);
        }
        size = 1;
    }
};

} // namespace aph

namespace aph
{
ShaderReflector::ShaderReflector(ReflectRequest request)
    : m_request(std::move(request))
{
    reflect();

    const vk::ImmutableSamplerBank* samplerBank = m_request.samplerBank;

    auto& combinedSetInfos = m_combinedLayout.setInfos;
    if (m_stageLayouts.contains(ShaderStage::VS))
    {
        const auto& shaderLayout = m_stageLayouts[ShaderStage::VS];
        VertexInput& vertexInput = m_vertexInput;
        uint32_t size = 0;
        aph::utils::forEachBit(
            shaderLayout.inputMask,
            [&](uint32_t location)
            {
                const auto& attr = shaderLayout.vertexAttributes[location];
                vertexInput.attributes.push_back(
                    { .location = location, .binding = attr.binding, .format = attr.format, .offset = attr.offset });
                size += attr.size;
            });
        vertexInput.bindings.push_back({ size });
    }

    for (unsigned set = 0; set < VULKAN_NUM_DESCRIPTOR_SETS; set++)
    {
        CombinedResourceLayout::SetInfo& setInfo = combinedSetInfos[set];
        std::array<::vk::Sampler, VULKAN_NUM_BINDINGS> vkImmutableSamplers{};

        const auto& pImmutableSamplers = samplerBank->samplers[set];
        const auto& shaderLayout = setInfo.shaderLayout;
        const auto& stageForBinds = setInfo.stagesForBindings;

        SmallVector<::vk::DescriptorSetLayoutBinding>& vkBindings = setInfos[set].bindings;
        SmallVector<::vk::DescriptorPoolSize>& poolSizes = setInfos[set].poolSizes;

        for (unsigned binding = 0; binding < VULKAN_NUM_BINDINGS; binding++)
        {
            const auto stages = static_cast<::vk::ShaderStageFlags>(stageForBinds[binding]);
            if (!stages)
            {
                continue;
            }

            unsigned arraySize = shaderLayout.arraySize[binding];
            unsigned poolArraySize;
            if (arraySize == ShaderLayout::UNSIZED_ARRAY)
            {
                arraySize = VULKAN_NUM_BINDINGS_BINDLESS_VARYING;
                poolArraySize = arraySize;
            }
            else
            {
                poolArraySize = arraySize * VULKAN_NUM_SETS_PER_POOL;
            }

            unsigned types = 0;
            if (shaderLayout.sampledImageMask.test(binding))
            {
                if ((shaderLayout.immutableSamplerMask.test(binding)) && pImmutableSamplers &&
                    pImmutableSamplers[binding])
                {
                    vkImmutableSamplers[binding] = pImmutableSamplers[binding]->getHandle();
                }

                vkBindings.push_back(
                    { binding, ::vk::DescriptorType::eCombinedImageSampler, arraySize, stages,
                      vkImmutableSamplers[binding] != ::vk::Sampler{} ? &vkImmutableSamplers[binding] : nullptr });
                poolSizes.push_back({ ::vk::DescriptorType::eCombinedImageSampler, poolArraySize });
                types++;
            }

            if (shaderLayout.sampledTexelBufferMask.test(binding))
            {
                vkBindings.push_back(
                    { binding, ::vk::DescriptorType::eUniformTexelBuffer, arraySize, stages, nullptr });
                poolSizes.push_back({ ::vk::DescriptorType::eUniformTexelBuffer, poolArraySize });
                types++;
            }

            if (shaderLayout.storageTexelBufferMask.test(binding))
            {
                vkBindings.push_back(
                    { binding, ::vk::DescriptorType::eStorageTexelBuffer, arraySize, stages, nullptr });
                poolSizes.push_back({ ::vk::DescriptorType::eStorageTexelBuffer, poolArraySize });
                types++;
            }

            if (shaderLayout.storageImageMask.test(binding))
            {
                vkBindings.push_back({ binding, ::vk::DescriptorType::eStorageImage, arraySize, stages, nullptr });
                poolSizes.push_back({ ::vk::DescriptorType::eStorageImage, poolArraySize });
                types++;
            }

            if (shaderLayout.uniformBufferMask.test(binding))
            {
                vkBindings.push_back({ binding, ::vk::DescriptorType::eUniformBufferDynamic, arraySize, stages, nullptr });
                poolSizes.push_back({ ::vk::DescriptorType::eUniformBufferDynamic, poolArraySize });
                types++;
            }

            if (shaderLayout.storageBufferMask.test(binding))
            {
                vkBindings.push_back({ binding, ::vk::DescriptorType::eStorageBuffer, arraySize, stages, nullptr });
                poolSizes.push_back({ ::vk::DescriptorType::eStorageBuffer, poolArraySize });
                types++;
            }

            if (shaderLayout.inputAttachmentMask.test(binding))
            {
                vkBindings.push_back({ binding, ::vk::DescriptorType::eInputAttachment, arraySize, stages, nullptr });
                poolSizes.push_back({ ::vk::DescriptorType::eInputAttachment, poolArraySize });
                types++;
            }

            if (shaderLayout.separateImageMask.test(binding))
            {
                vkBindings.push_back({ binding, ::vk::DescriptorType::eSampledImage, arraySize, stages, nullptr });
                poolSizes.push_back({ ::vk::DescriptorType::eSampledImage, poolArraySize });
                types++;
            }

            if (shaderLayout.samplerMask.test(binding))
            {
                if ((shaderLayout.immutableSamplerMask.test(binding)) && pImmutableSamplers &&
                    pImmutableSamplers[binding])
                {
                    vkImmutableSamplers[binding] = pImmutableSamplers[binding]->getHandle();
                }

                vkBindings.push_back(
                    { binding, ::vk::DescriptorType::eSampler, arraySize, stages,
                      vkImmutableSamplers[binding] != ::vk::Sampler{} ? &vkImmutableSamplers[binding] : nullptr });
                poolSizes.push_back({ ::vk::DescriptorType::eSampler, poolArraySize });
                types++;
            }

            (void)types;
            APH_ASSERT(types <= 1 && "Descriptor set aliasing!");
        }
    }
}

ResourceLayout ShaderReflector::reflectStageLayout(const std::vector<uint32_t>& spvCode)
{
    spirv_cross::Compiler compiler{ spvCode.data(), spvCode.size() };
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    ResourceLayout layout{};
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
            .format = vk::utils::getFormatFromVk(format),
            .size = static_cast<uint32_t>(getTypeSize(type)),
        };
    }

    uint32_t attrOffset = 0;
    aph::utils::forEachBit(layout.inputMask,
                           [&](uint32_t location)
                           {
                               auto& attr = layout.vertexAttributes[location];
                               attr.offset = attrOffset;
                               attrOffset += attr.size;
                           });

    for (const auto& res : resources.stage_outputs)
    {
        auto location = compiler.get_decoration(res.id, spv::DecorationLocation);
        layout.outputMask.set(location);
    }
    for (const auto& res : resources.uniform_buffers)
    {
        uint32_t set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
        uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);
        APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
        APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

        layout.layouts[set].uniformBufferMask.set(binding);
        updateArrayInfo(layout, compiler.get_type(res.type_id), set, binding);
    }
    for (const auto& res : resources.storage_buffers)
    {
        unsigned set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
        unsigned binding = compiler.get_decoration(res.id, spv::DecorationBinding);
        APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
        APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

        layout.layouts[set].storageBufferMask.set(binding);
        updateArrayInfo(layout, compiler.get_type(res.type_id), set, binding);
    }
    for (const auto& res : resources.storage_images)
    {
        unsigned set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
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
    for (const auto& res : resources.sampled_images)
    {
        unsigned set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
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
    for (const auto& res : resources.separate_images)
    {
        unsigned set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
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
    for (const auto& res : resources.separate_samplers)
    {
        unsigned set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
        unsigned binding = compiler.get_decoration(res.id, spv::DecorationBinding);
        APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
        APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

        layout.layouts[set].samplerMask.set(binding);
        updateArrayInfo(layout, compiler.get_type(res.type_id), set, binding);
    }

    if (!resources.push_constant_buffers.empty())
    {
        layout.pushConstantSize =
            compiler.get_declared_struct_size(compiler.get_type(resources.push_constant_buffers.front().base_type_id));
    }

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

    return layout;
}

void ShaderReflector::reflect()
{
    const std::vector<vk::Shader*>& shaders = m_request.shaders;
    const vk::ImmutableSamplerBank* samplerBank = m_request.samplerBank;

    vk::ImmutableSamplerBank extImmutableSamplers = {};

    auto& combinedSetInfos = m_combinedLayout.setInfos;

    for (const auto& shader : shaders)
    {
        APH_ASSERT(shader);
        const auto& stage = shader->getStage();
        const auto& shaderLayout = reflectStageLayout(shader->getCode());
        m_stageLayouts[stage] = shaderLayout;

        {
            if (stage == ShaderStage::VS)
            {
                m_combinedLayout.attributeMask = shaderLayout.inputMask;
                for (auto idx = 0; idx < VULKAN_NUM_VERTEX_ATTRIBS; ++idx)
                {
                    m_combinedLayout.vertexAttr[idx] = shaderLayout.vertexAttributes[idx];
                }
            }

            if (stage == ShaderStage::FS)
            {
                m_combinedLayout.renderTargetMask = shaderLayout.outputMask;
            }
        }

        auto stageMask = vk::utils::VkCast(stage);
        for (unsigned i = 0; i < VULKAN_NUM_DESCRIPTOR_SETS; i++)
        {
            CombinedResourceLayout::SetInfo& combinedSetInfo = combinedSetInfos[i];

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

            auto activeBinds = shaderLayout.layouts[i].sampledImageMask | shaderLayout.layouts[i].storageImageMask |
                               shaderLayout.layouts[i].uniformBufferMask | shaderLayout.layouts[i].storageBufferMask |
                               shaderLayout.layouts[i].sampledTexelBufferMask |
                               shaderLayout.layouts[i].storageTexelBufferMask |
                               shaderLayout.layouts[i].inputAttachmentMask | shaderLayout.layouts[i].samplerMask |
                               shaderLayout.layouts[i].separateImageMask;

            if (activeBinds.any())
            {
                combinedSetInfo.stagesForSets |= stageMask;
            }

            aph::utils::forEachBit(activeBinds,
                                   [&](uint32_t bit)
                                   {
                                       combinedSetInfo.stagesForBindings[bit] |= stageMask;

                                       auto& combinedSize = combinedSetInfo.shaderLayout.arraySize[bit];
                                       auto& shaderSize = shaderLayout.layouts[i].arraySize[bit];
                                       if (combinedSize && combinedSize != shaderSize)
                                       {
                                           VK_LOG_ERR("Mismatch between array sizes in different shaders.");
                                           APH_ASSERT(false);
                                       }
                                       else
                                       {
                                           combinedSize = shaderSize;
                                       }
                                   });
        }

        // Merge push constant ranges into one range.
        // Do not try to split into multiple ranges as it just complicates things for no obvious gain.
        if (shaderLayout.pushConstantSize != 0)
        {
            m_combinedLayout.pushConstantRange.stageFlags |= stageMask;
            m_combinedLayout.pushConstantRange.size =
                std::max(m_combinedLayout.pushConstantRange.size, shaderLayout.pushConstantSize);
        }

        m_combinedLayout.specConstantMask[stage] = shaderLayout.specConstantMask;
        m_combinedLayout.combinedSpecConstantMask |= shaderLayout.specConstantMask;
        m_combinedLayout.bindlessDescriptorSetMask |= shaderLayout.bindlessSetMask;
    }

    for (unsigned setIdx = 0; setIdx < VULKAN_NUM_DESCRIPTOR_SETS; setIdx++)
    {
        CombinedResourceLayout::SetInfo& setInfo = combinedSetInfos[setIdx];

        if (samplerBank)
        {
            aph::utils::forEachBit(setInfo.shaderLayout.samplerMask | setInfo.shaderLayout.sampledImageMask,
                                   [&](uint32_t binding)
                                   {
                                       if (samplerBank->samplers[setIdx][binding])
                                       {
                                           extImmutableSamplers.samplers[setIdx][binding] =
                                               samplerBank->samplers[setIdx][binding];
                                           setInfo.shaderLayout.immutableSamplerMask.set(binding);
                                       }
                                   });
        }

        if (setInfo.stagesForSets)
        {
            m_combinedLayout.descriptorSetMask.set(setIdx);

            for (unsigned binding = 0; binding < VULKAN_NUM_BINDINGS; binding++)
            {
                auto& arraySize = setInfo.shaderLayout.arraySize[binding];
                if (arraySize == ShaderLayout::UNSIZED_ARRAY)
                {
                    // Allows us to have one unified descriptor set layout for bindless.
                    setInfo.stagesForBindings[binding] = ::vk::ShaderStageFlagBits::eAll;
                }
                else if (arraySize == 0)
                {
                    arraySize = 1;
                }
                else
                {
                    for (unsigned i = 1; i < arraySize; i++)
                    {
                        if (setInfo.stagesForBindings[binding + i])
                        {
                            VK_LOG_ERR(
                                "Detected binding aliasing for (%u, %u). Binding array with %u elements starting "
                                "at (%u, "
                                "%u) overlaps.\n",
                                i, binding + i, arraySize, i, binding);
                        }
                    }
                }
            }
        }
    }
}

SmallVector<::vk::DescriptorSetLayoutBinding> ShaderReflector::getLayoutBindings(uint32_t set)
{
    return setInfos[set].bindings;
}

SmallVector<::vk::DescriptorPoolSize> ShaderReflector::getPoolSizes(uint32_t set)
{
    return setInfos[set].poolSizes;
}
} // namespace aph
