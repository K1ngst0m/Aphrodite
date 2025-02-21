#include "shaderReflector.h"

#include "api/vulkan/device.h"
#include "api/vulkan/sampler.h"
#include "spirv_cross.hpp"

namespace aph
{
std::unordered_map<spirv_cross::SPIRType::BaseType, size_t> baseTypeSizeMap = {
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
    size_t baseSize = baseTypeSizeMap[type.basetype];

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
    auto& size = layout.setShaderLayouts[set].arraySize[binding];
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

vk::ResourceLayout ReflectLayout(const std::vector<uint32_t>& spvCode)
{
    spirv_cross::Compiler compiler{spvCode.data(), spvCode.size()};
    auto                  resources = compiler.get_shader_resources();

    vk::ResourceLayout layout{};
    for(const auto& res : resources.stage_inputs)
    {
        auto location = compiler.get_decoration(res.id, spv::DecorationLocation);
        layout.inputMask |= 1u << location;
        const spirv_cross::SPIRType& type = compiler.get_type(res.type_id);

        VkFormat format = spirTypeToVkFormat(type);

        layout.vertexAttr[location] = {
            // TODO
            .binding = 0,
            .format  = format,
            .size    = static_cast<uint32_t>(getTypeSize(type)),
        };
    }

    uint32_t attrOffset = 0;
    aph::utils::forEachBit(layout.inputMask, [&](uint32_t location) {
        auto& attr  = layout.vertexAttr[location];
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

        layout.setShaderLayouts[set].uniformBufferMask |= 1u << binding;
        updateArrayInfo(layout, compiler.get_type(res.type_id), set, binding);
    }
    for(const auto& res : resources.storage_buffers)
    {
        unsigned set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
        unsigned binding = compiler.get_decoration(res.id, spv::DecorationBinding);
        APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
        APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

        layout.setShaderLayouts[set].storageBufferMask |= 1u << binding;
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
            layout.setShaderLayouts[set].storageTexelBufferMask |= 1u << binding;
        }
        else
        {
            layout.setShaderLayouts[set].storageImageMask |= 1u << binding;
        }

        if(compiler.get_type(type.image.type).basetype == spirv_cross::SPIRType::BaseType::Float)
        {
            layout.setShaderLayouts[set].fpMask |= 1u << binding;
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
            layout.setShaderLayouts[set].fpMask |= 1u << binding;
        }
        if(type.image.dim == spv::DimBuffer)
        {
            layout.setShaderLayouts[set].sampledTexelBufferMask |= 1u << binding;
        }
        else
        {
            layout.setShaderLayouts[set].sampledImageMask |= 1u << binding;
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
            layout.setShaderLayouts[set].fpMask |= 1u << binding;
        }
        if(type.image.dim == spv::DimBuffer)
        {
            layout.setShaderLayouts[set].sampledTexelBufferMask |= 1u << binding;
        }
        else
        {
            layout.setShaderLayouts[set].separateImageMask |= 1u << binding;
        }
        updateArrayInfo(layout, type, set, binding);
    }
    for(const auto& res : resources.separate_samplers)
    {
        unsigned set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
        unsigned binding = compiler.get_decoration(res.id, spv::DecorationBinding);
        APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
        APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

        layout.setShaderLayouts[set].samplerMask |= 1u << binding;
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
}  // namespace aph
