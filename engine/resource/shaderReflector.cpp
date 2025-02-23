#include "shaderReflector.h"
#include "spirv_cross.hpp"

namespace aph
{
const std::unordered_map<spirv_cross::SPIRType::BaseType, size_t> baseTypeSizeMap = {
    {spirv_cross::SPIRType::Float, 4},
    {spirv_cross::SPIRType::Int, 4},
    {spirv_cross::SPIRType::UInt, 4},
    {spirv_cross::SPIRType::Double, 8},
    // TODO Add other base types as needed
};

std::size_t getTypeSize(const spirv_cross::SPIRType& type)
{
    // Lookup base size from the map
    APH_ASSERT(baseTypeSizeMap.contains(type.basetype));
    size_t baseSize = baseTypeSizeMap.at(type.basetype);

    // Calculate size for vectors and matrices
    size_t elementCount = type.vecsize * type.columns;
    size_t size         = baseSize * elementCount;

    // Handle arrays
    if(!type.array.empty())
    {
        size_t arraySize = 1;
        for(size_t length : type.array)
            arraySize *= length;
        size *= arraySize;
    }

    return size;
}

VkFormat spirTypeToVkFormat(const spirv_cross::SPIRType& type)
{
    // Handle vector types
    if(type.vecsize == 1)
    {  // Scalars
        switch(type.basetype)
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
    else if(type.vecsize == 2)
    {  // 2-component vectors
        switch(type.basetype)
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
    else if(type.vecsize == 3)
    {  // 3-component vectors
        switch(type.basetype)
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
    else if(type.vecsize == 4)
    {  // 4-component vectors
        switch(type.basetype)
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

void updateArrayInfo(aph::vk::ResourceLayout& layout, const spirv_cross::SPIRType& type, unsigned set, unsigned binding)
{
    auto& size = layout.shaderLayouts[set].arraySize[binding];
    if(!type.array.empty())
    {
        if(type.array.size() != 1)
        {
            VK_LOG_ERR("Array dimension must be 1.");
        }
        else if(!type.array_size_literal.front())
        {
            VK_LOG_ERR("Array dimension must be a literal.");
        }
        else
        {
            if(type.array.front() == 0)
            {
                if(binding != 0)
                {
                    VK_LOG_ERR("Bindless textures can only be used with binding = 0 in a set.");
                }

                if(type.basetype != spirv_cross::SPIRType::Image || type.image.dim == spv::DimBuffer)
                {
                    VK_LOG_ERR("Can only use bindless for sampled images.");
                }
                else
                {
                    layout.bindlessSetMask |= 1u << set;
                }

                size = vk::ShaderLayout::UNSIZED_ARRAY;
            }
            else if(size && size != type.array.front())
            {
                VK_LOG_ERR("Array dimension for (%u, %u) is inconsistent.", set, binding);
            }
            else if(type.array.front() + binding > VULKAN_NUM_BINDINGS)
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
        if(size && size != 1)
        {
            VK_LOG_ERR("Array dimension for (%u, %u) is inconsistent.", set, binding);
        }
        size = 1;
    }
};

vk::ResourceLayout reflectLayout(const std::vector<uint32_t>& spvCode)
{
    spirv_cross::Compiler compiler{spvCode.data(), spvCode.size()};
    spirv_cross::ShaderResources       resources = compiler.get_shader_resources();

    vk::ResourceLayout layout{};
    for(const auto& res : resources.stage_inputs)
    {
        uint32_t location = compiler.get_decoration(res.id, spv::DecorationLocation);
        layout.inputMask |= 1u << location;
        APH_ASSERT(layout.inputMask.size() > location);
        const spirv_cross::SPIRType& type = compiler.get_type(res.type_id);

        VkFormat format = spirTypeToVkFormat(type);

        layout.vertexAttributes[location] = {
            // TODO multiple bindings
            .binding = 0,
            .format  = format,
            .size    = static_cast<uint32_t>(getTypeSize(type)),
        };
    }

    uint32_t attrOffset = 0;
    aph::utils::forEachBit(layout.inputMask, [&](uint32_t location) {
        auto& attr  = layout.vertexAttributes[location];
        attr.offset = attrOffset;
        attrOffset += attr.size;
    });

    for(const auto& res : resources.stage_outputs)
    {
        auto location = compiler.get_decoration(res.id, spv::DecorationLocation);
        layout.outputMask |= 1u << location;
    }
    for(const auto& res : resources.uniform_buffers)
    {
        uint32_t set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
        uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);
        APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
        APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

        layout.shaderLayouts[set].uniformBufferMask |= 1u << binding;
        updateArrayInfo(layout, compiler.get_type(res.type_id), set, binding);
    }
    for(const auto& res : resources.storage_buffers)
    {
        unsigned set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
        unsigned binding = compiler.get_decoration(res.id, spv::DecorationBinding);
        APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
        APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

        layout.shaderLayouts[set].storageBufferMask |= 1u << binding;
        updateArrayInfo(layout, compiler.get_type(res.type_id), set, binding);
    }
    for(const auto& res : resources.storage_images)
    {
        unsigned set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
        unsigned binding = compiler.get_decoration(res.id, spv::DecorationBinding);
        APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
        APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

        auto& type = compiler.get_type(res.type_id);
        if(type.image.dim == spv::DimBuffer)
        {
            layout.shaderLayouts[set].storageTexelBufferMask |= 1u << binding;
        }
        else
        {
            layout.shaderLayouts[set].storageImageMask |= 1u << binding;
        }

        if(compiler.get_type(type.image.type).basetype == spirv_cross::SPIRType::BaseType::Float)
        {
            layout.shaderLayouts[set].fpMask |= 1u << binding;
        }

        updateArrayInfo(layout, type, set, binding);
    }
    for(const auto& res : resources.sampled_images)
    {
        unsigned set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
        unsigned binding = compiler.get_decoration(res.id, spv::DecorationBinding);
        APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
        APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

        auto& type = compiler.get_type(res.type_id);
        if(compiler.get_type(type.image.type).basetype == spirv_cross::SPIRType::BaseType::Float)
        {
            layout.shaderLayouts[set].fpMask |= 1u << binding;
        }
        if(type.image.dim == spv::DimBuffer)
        {
            layout.shaderLayouts[set].sampledTexelBufferMask |= 1u << binding;
        }
        else
        {
            layout.shaderLayouts[set].sampledImageMask |= 1u << binding;
        }
        updateArrayInfo(layout, type, set, binding);
    }
    for(const auto& res : resources.separate_images)
    {
        unsigned set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
        unsigned binding = compiler.get_decoration(res.id, spv::DecorationBinding);
        APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
        APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

        auto& type = compiler.get_type(res.type_id);
        if(compiler.get_type(type.image.type).basetype == spirv_cross::SPIRType::BaseType::Float)
        {
            layout.shaderLayouts[set].fpMask |= 1u << binding;
        }
        if(type.image.dim == spv::DimBuffer)
        {
            layout.shaderLayouts[set].sampledTexelBufferMask |= 1u << binding;
        }
        else
        {
            layout.shaderLayouts[set].separateImageMask |= 1u << binding;
        }
        updateArrayInfo(layout, type, set, binding);
    }
    for(const auto& res : resources.separate_samplers)
    {
        unsigned set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
        unsigned binding = compiler.get_decoration(res.id, spv::DecorationBinding);
        APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
        APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

        layout.shaderLayouts[set].samplerMask |= 1u << binding;
        updateArrayInfo(layout, compiler.get_type(res.type_id), set, binding);
    }

    if(!resources.push_constant_buffers.empty())
    {
        layout.pushConstantSize =
            compiler.get_declared_struct_size(compiler.get_type(resources.push_constant_buffers.front().base_type_id));
    }

    auto specConstants = compiler.get_specialization_constants();
    for(auto& c : specConstants)
    {
        if(c.constant_id >= VULKAN_NUM_TOTAL_SPEC_CONSTANTS)
        {
            VK_LOG_ERR("Spec constant ID: %u is out of range, will be ignored.", c.constant_id);
            continue;
        }

        layout.specConstantMask |= 1u << c.constant_id;
    }

    return layout;
}

vk::CombinedResourceLayout combineLayout(const std::vector<vk::Shader*>& shaders, const vk::ImmutableSamplerBank* samplerBank)
{
    vk::CombinedResourceLayout programLayout{};

    for(const auto& shader : shaders)
    {
        if(shader->getStage() == ShaderStage::VS)
        {
            programLayout.attributeMask = shader->getLayout().inputMask;
            for(auto idx = 0; idx < VULKAN_NUM_VERTEX_ATTRIBS; ++idx)
            {
                programLayout.vertexAttr[idx] = shader->getLayout().vertexAttributes[idx];
            }
        }
        if(shader->getStage() == ShaderStage::FS)
        {
            programLayout.renderTargetMask = shader->getLayout().outputMask;
        }
    }

    vk::ImmutableSamplerBank extImmutableSamplers = {};

    for(const auto& shader : shaders)
    {
        APH_ASSERT(shader);

        const auto& stage = shader->getStage();
        auto&    shaderLayout = shader->getLayout();
        uint32_t stageMask    = vk::utils::VkCast(stage);

        for(unsigned i = 0; i < VULKAN_NUM_DESCRIPTOR_SETS; i++)
        {
            programLayout.setInfos[i].shaderLayout.sampledImageMask |=
                shaderLayout.shaderLayouts[i].sampledImageMask;
            programLayout.setInfos[i].shaderLayout.storageImageMask |=
                shaderLayout.shaderLayouts[i].storageImageMask;
            programLayout.setInfos[i].shaderLayout.uniformBufferMask |=
                shaderLayout.shaderLayouts[i].uniformBufferMask;
            programLayout.setInfos[i].shaderLayout.storageBufferMask |=
                shaderLayout.shaderLayouts[i].storageBufferMask;
            programLayout.setInfos[i].shaderLayout.sampledTexelBufferMask |=
                shaderLayout.shaderLayouts[i].sampledTexelBufferMask;
            programLayout.setInfos[i].shaderLayout.storageTexelBufferMask |=
                shaderLayout.shaderLayouts[i].storageTexelBufferMask;
            programLayout.setInfos[i].shaderLayout.inputAttachmentMask |=
                shaderLayout.shaderLayouts[i].inputAttachmentMask;
            programLayout.setInfos[i].shaderLayout.samplerMask |= shaderLayout.shaderLayouts[i].samplerMask;
            programLayout.setInfos[i].shaderLayout.separateImageMask |=
                shaderLayout.shaderLayouts[i].separateImageMask;
            programLayout.setInfos[i].shaderLayout.fpMask |= shaderLayout.shaderLayouts[i].fpMask;

            uint32_t activeBinds =
                shaderLayout.shaderLayouts[i].sampledImageMask | shaderLayout.shaderLayouts[i].storageImageMask |
                shaderLayout.shaderLayouts[i].uniformBufferMask |
                shaderLayout.shaderLayouts[i].storageBufferMask |
                shaderLayout.shaderLayouts[i].sampledTexelBufferMask |
                shaderLayout.shaderLayouts[i].storageTexelBufferMask |
                shaderLayout.shaderLayouts[i].inputAttachmentMask | shaderLayout.shaderLayouts[i].samplerMask |
                shaderLayout.shaderLayouts[i].separateImageMask;

            if(activeBinds)
            {
                programLayout.setInfos[i].stagesForSets |= stageMask;
            }

            aph::utils::forEachBit(activeBinds, [&](uint32_t bit) {
                programLayout.setInfos[i].stagesForBindings[bit] |= stageMask;

                auto& combinedSize = programLayout.setInfos[i].shaderLayout.arraySize[bit];
                auto& shaderSize   = shaderLayout.shaderLayouts[i].arraySize[bit];
                if(combinedSize && combinedSize != shaderSize)
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
        if(shaderLayout.pushConstantSize != 0)
        {
            programLayout.pushConstantRange.stageFlags |= stageMask;
            programLayout.pushConstantRange.size =
                std::max(programLayout.pushConstantRange.size, shaderLayout.pushConstantSize);
        }

        programLayout.specConstantMask[stage] = shaderLayout.specConstantMask;
        programLayout.combinedSpecConstantMask |= shaderLayout.specConstantMask;
        // TODO
        // programLayout.bindlessDescriptorSetMask |= shaderLayout.bindlessSetMask;
    }

    if(samplerBank)
    {
        for(unsigned i = 0; i < VULKAN_NUM_DESCRIPTOR_SETS; i++)
        {
            aph::utils::forEachBit(programLayout.setInfos[i].shaderLayout.samplerMask |
                                       programLayout.setInfos[i].shaderLayout.sampledImageMask,
                                   [&](uint32_t binding) {
                                       if(samplerBank->samplers[i][binding])
                                       {
                                           extImmutableSamplers.samplers[i][binding] =
                                               samplerBank->samplers[i][binding];
                                           programLayout.setInfos[i].shaderLayout.immutableSamplerMask |= 1u << binding;
                                       }
                                   });
        }
    }

    for(unsigned setIdx = 0; setIdx < VULKAN_NUM_DESCRIPTOR_SETS; setIdx++)
    {
        if(programLayout.setInfos[setIdx].stagesForSets != 0)
        {
            programLayout.descriptorSetMask |= 1u << setIdx;

            for(unsigned binding = 0; binding < VULKAN_NUM_BINDINGS; binding++)
            {
                auto& arraySize = programLayout.setInfos[setIdx].shaderLayout.arraySize[binding];
                if(arraySize == vk::ShaderLayout::UNSIZED_ARRAY)
                {
                    for(unsigned i = 1; i < VULKAN_NUM_BINDINGS; i++)
                    {
                        if(programLayout.setInfos[i].stagesForBindings[i] != 0)
                        {
                            VK_LOG_ERR("Using bindless for set = %u, but binding = %u has a descriptor attached to it.",
                                       i, i);
                        }
                    }

                    // Allows us to have one unified descriptor set layout for bindless.
                    programLayout.setInfos[setIdx].stagesForBindings[binding] = VK_SHADER_STAGE_ALL;
                }
                else if(arraySize == 0)
                {
                    arraySize = 1;
                }
                else
                {
                    for(unsigned i = 1; i < arraySize; i++)
                    {
                        if(programLayout.setInfos[i].stagesForBindings[binding + i] != 0)
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

    return programLayout;
}

VertexInput getVertexInputInfo(vk::CombinedResourceLayout combineLayout)
{
    VertexInput vertexInput;
    uint32_t    size = 0;
    aph::utils::forEachBit(combineLayout.attributeMask, [&](uint32_t location) {
        auto& attr = combineLayout.vertexAttr[location];
        vertexInput.attributes.push_back({.location = location,
                                          .binding  = 0,
                                          .format   = vk::utils::getFormatFromVk(attr.format),
                                          .offset   = attr.offset});
        size += attr.size;
    });
    vertexInput.bindings.push_back({size});
    return vertexInput;
}
}  // namespace aph
