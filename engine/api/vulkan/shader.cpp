#include "shader.h"
#include "device.h"

namespace aph::vk
{
// create descriptor set layout
vk::DescriptorSetLayout* createDescriptorSetLayout(vk::Device* m_pDevice, const vk::ShaderLayout& layout,
                                                   const vk::Sampler* const*          pImmutableSamplers,
                                                   const uint32_t*                    stageForBinds,
                                                   SmallVector<VkDescriptorPoolSize>& poolSize)
{
    VkSampler                                 vkImmutableSamplers[VULKAN_NUM_BINDINGS] = {};
    SmallVector<VkDescriptorSetLayoutBinding> vkBindings;

    // VkDescriptorBindingFlagsEXT               binding_flags = 0;
    // VkDescriptorSetLayoutBindingFlagsCreateInfoEXT flags = {
    //     VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT};

    for(unsigned i = 0; i < VULKAN_NUM_BINDINGS; i++)
    {
        uint32_t stages = stageForBinds[i];
        if(stages == 0)
            continue;

        unsigned arraySize = layout.arraySize[i];
        unsigned poolArraySize;
        if(arraySize == vk::ShaderLayout::UNSIZED_ARRAY)
        {
            arraySize     = VULKAN_NUM_BINDINGS_BINDLESS_VARYING;
            poolArraySize = arraySize;
        }
        else
            poolArraySize = arraySize * VULKAN_NUM_SETS_PER_POOL;

        unsigned types = 0;
        if(layout.sampledImageMask & (1u << i))
        {
            if((layout.immutableSamplerMask & (1u << i)) && pImmutableSamplers && pImmutableSamplers[i])
                vkImmutableSamplers[i] = pImmutableSamplers[i]->getHandle();

            vkBindings.push_back({i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, arraySize, stages,
                                  vkImmutableSamplers[i] != VK_NULL_HANDLE ? &vkImmutableSamplers[i] : nullptr});
            poolSize.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, poolArraySize});
            types++;
        }

        if(layout.sampledTexelBufferMask & (1u << i))
        {
            vkBindings.push_back({i, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, arraySize, stages, nullptr});
            poolSize.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, poolArraySize});
            types++;
        }

        if(layout.storageTexelBufferMask & (1u << i))
        {
            vkBindings.push_back({i, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, arraySize, stages, nullptr});
            poolSize.push_back({VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, poolArraySize});
            types++;
        }

        if(layout.storageImageMask & (1u << i))
        {
            vkBindings.push_back({i, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, arraySize, stages, nullptr});
            poolSize.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, poolArraySize});
            types++;
        }

        if(layout.uniformBufferMask & (1u << i))
        {
            vkBindings.push_back({i, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, arraySize, stages, nullptr});
            poolSize.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, poolArraySize});
            types++;
        }

        if(layout.storageBufferMask & (1u << i))
        {
            vkBindings.push_back({i, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, arraySize, stages, nullptr});
            poolSize.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, poolArraySize});
            types++;
        }

        if(layout.inputAttachmentMask & (1u << i))
        {
            vkBindings.push_back({i, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, arraySize, stages, nullptr});
            poolSize.push_back({VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, poolArraySize});
            types++;
        }

        if(layout.separateImageMask & (1u << i))
        {
            vkBindings.push_back({i, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, arraySize, stages, nullptr});
            poolSize.push_back({VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, poolArraySize});
            types++;
        }

        if(layout.samplerMask & (1u << i))
        {
            if((layout.immutableSamplerMask & (1u << i)) && pImmutableSamplers && pImmutableSamplers[i])
                vkImmutableSamplers[i] = pImmutableSamplers[i]->getHandle();

            vkBindings.push_back({i, VK_DESCRIPTOR_TYPE_SAMPLER, arraySize, stages,
                                  vkImmutableSamplers[i] != VK_NULL_HANDLE ? &vkImmutableSamplers[i] : nullptr});
            poolSize.push_back({VK_DESCRIPTOR_TYPE_SAMPLER, poolArraySize});
            types++;
        }

        (void)types;
        APH_ASSERT(types <= 1 && "Descriptor set aliasing!");
    }

    VkDescriptorSetLayoutCreateInfo info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    if(!vkBindings.empty())
    {
        info.bindingCount = vkBindings.size();
        info.pBindings    = vkBindings.data();
    }

#ifdef APH_DEBUG
    VK_LOG_DEBUG("Creating descriptor set layout.");
#endif
    vk::DescriptorSetLayout* setLayout{};
    {
        VkDescriptorSetLayout vkSetLayout;
        _VR(m_pDevice->getDeviceTable()->vkCreateDescriptorSetLayout(m_pDevice->getHandle(), &info, vk::vkAllocator(),
                                                                     &vkSetLayout));
        setLayout = new vk::DescriptorSetLayout(m_pDevice, info, vkSetLayout);
    }
    return setLayout;
};
}  // namespace aph::vk

namespace aph::vk
{

Shader::Shader(const CreateInfoType& createInfo, HandleType handle) : ResourceHandle(handle, createInfo)
{
}

ShaderProgram::ShaderProgram(const CreateInfoType& createInfo) : ResourceHandle({}, createInfo), m_pDevice(createInfo.pDevice)
{
    switch(getPipelineType())
    {
    case PipelineType::Geometry:
    {
        APH_ASSERT(createInfo.geometry.pVertex);
        APH_ASSERT(createInfo.geometry.pFragment);
        m_shaders[ShaderStage::VS] = createInfo.geometry.pVertex;
        m_shaders[ShaderStage::FS] = createInfo.geometry.pFragment;
    }
    break;
    case PipelineType::Mesh:
    {
        APH_ASSERT(createInfo.mesh.pMesh);
        APH_ASSERT(createInfo.mesh.pFragment);
        m_shaders[ShaderStage::MS] = createInfo.mesh.pMesh;
        if(createInfo.mesh.pTask)
        {
            m_shaders[ShaderStage::TS] = createInfo.mesh.pTask;
        }
        m_shaders[ShaderStage::FS] = createInfo.mesh.pFragment;
    }
    break;
    case PipelineType::Compute:
    {
        m_shaders[ShaderStage::CS] = createInfo.compute.pCompute;
    }
    break;
    case PipelineType::Undefined:
    case PipelineType::RayTracing:
        break;
    }
    combineLayout(createInfo.samplerBank);
    createPipelineLayout(createInfo.samplerBank);
    createVertexInput();
}

void ShaderProgram::combineLayout(const ImmutableSamplerBank* samplerBank)
{
    CombinedResourceLayout programLayout{};
    if(m_shaders.contains(ShaderStage::VS))
    {
        programLayout.attributeMask = m_shaders[ShaderStage::VS]->getResourceLayout().inputMask;
        for(auto idx = 0; idx < VULKAN_NUM_VERTEX_ATTRIBS; ++idx)
        {
            programLayout.vertexAttr[idx] = m_shaders[ShaderStage::VS]->getResourceLayout().vertexAttr[idx];
        }
    }
    if(m_shaders.contains(ShaderStage::FS) && m_shaders[ShaderStage::FS])
    {
        programLayout.renderTargetMask = m_shaders[ShaderStage::FS]->getResourceLayout().outputMask;
    }

    ImmutableSamplerBank extImmutableSamplers = {};

    for(const auto& [stage, shader] : m_shaders)
    {
        APH_ASSERT(shader);
        auto&    shaderLayout = shader->getResourceLayout();
        uint32_t stageMask    = utils::VkCast(stage);

        for(unsigned i = 0; i < VULKAN_NUM_DESCRIPTOR_SETS; i++)
        {
            programLayout.setInfos[i].shaderLayout.sampledImageMask |=
                shaderLayout.setShaderLayouts[i].sampledImageMask;
            programLayout.setInfos[i].shaderLayout.storageImageMask |=
                shaderLayout.setShaderLayouts[i].storageImageMask;
            programLayout.setInfos[i].shaderLayout.uniformBufferMask |=
                shaderLayout.setShaderLayouts[i].uniformBufferMask;
            programLayout.setInfos[i].shaderLayout.storageBufferMask |=
                shaderLayout.setShaderLayouts[i].storageBufferMask;
            programLayout.setInfos[i].shaderLayout.sampledTexelBufferMask |=
                shaderLayout.setShaderLayouts[i].sampledTexelBufferMask;
            programLayout.setInfos[i].shaderLayout.storageTexelBufferMask |=
                shaderLayout.setShaderLayouts[i].storageTexelBufferMask;
            programLayout.setInfos[i].shaderLayout.inputAttachmentMask |=
                shaderLayout.setShaderLayouts[i].inputAttachmentMask;
            programLayout.setInfos[i].shaderLayout.samplerMask |= shaderLayout.setShaderLayouts[i].samplerMask;
            programLayout.setInfos[i].shaderLayout.separateImageMask |=
                shaderLayout.setShaderLayouts[i].separateImageMask;
            programLayout.setInfos[i].shaderLayout.fpMask |= shaderLayout.setShaderLayouts[i].fpMask;

            uint32_t activeBinds =
                shaderLayout.setShaderLayouts[i].sampledImageMask | shaderLayout.setShaderLayouts[i].storageImageMask |
                shaderLayout.setShaderLayouts[i].uniformBufferMask |
                shaderLayout.setShaderLayouts[i].storageBufferMask |
                shaderLayout.setShaderLayouts[i].sampledTexelBufferMask |
                shaderLayout.setShaderLayouts[i].storageTexelBufferMask |
                shaderLayout.setShaderLayouts[i].inputAttachmentMask | shaderLayout.setShaderLayouts[i].samplerMask |
                shaderLayout.setShaderLayouts[i].separateImageMask;

            if(activeBinds)
            {
                programLayout.setInfos[i].stagesForSets |= stageMask;
            }

            aph::utils::forEachBit(activeBinds, [&](uint32_t bit) {
                programLayout.setInfos[i].stagesForBindings[bit] |= stageMask;

                auto& combinedSize = programLayout.setInfos[i].shaderLayout.arraySize[bit];
                auto& shaderSize   = shaderLayout.setShaderLayouts[i].arraySize[bit];
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
                if(arraySize == ShaderLayout::UNSIZED_ARRAY)
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

    // TODO
    m_combineLayout = std::move(programLayout);
}

void ShaderProgram::createPipelineLayout(const ImmutableSamplerBank* samplerBank)
{
    m_pSetLayouts.resize(VULKAN_NUM_DESCRIPTOR_SETS);
    unsigned numSets = 0;
    for(unsigned i = 0; i < VULKAN_NUM_DESCRIPTOR_SETS; i++)
    {
        m_pSetLayouts[i] =
            createDescriptorSetLayout(m_pDevice, m_combineLayout.setInfos[i].shaderLayout, samplerBank->samplers[i],
                                      m_combineLayout.setInfos[i].stagesForBindings, m_poolSize);
        if(m_combineLayout.descriptorSetMask & (1u << i))
        {
            numSets = i + 1;
        }
    }

    if(numSets > m_pDevice->getPhysicalDevice()->getProperties().limits.maxBoundDescriptorSets)
    {
        VK_LOG_ERR("Number of sets %u exceeds device limit of %u.", numSets,
                   m_pDevice->getPhysicalDevice()->getProperties().limits.maxBoundDescriptorSets);
    }

    VkPipelineLayoutCreateInfo         info = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    SmallVector<VkDescriptorSetLayout> vkSetLayouts;
    if(numSets)
    {
        vkSetLayouts.reserve(m_pSetLayouts.size());
        for(const auto& setLayout : m_pSetLayouts)
        {
            vkSetLayouts.push_back(setLayout->getHandle());
        }
        info.setLayoutCount = numSets;
        info.pSetLayouts    = vkSetLayouts.data();
    }

    if(m_combineLayout.pushConstantRange.stageFlags != 0)
    {
        info.pushConstantRangeCount = 1;
        info.pPushConstantRanges    = &m_combineLayout.pushConstantRange;
    }

#ifdef APH_DEBUG
    VK_LOG_DEBUG("Creating pipeline layout.");
#endif

    auto table = m_pDevice->getDeviceTable();
    if(table->vkCreatePipelineLayout(m_pDevice->getHandle(), &info, vkAllocator(), &m_pipeLayout) != VK_SUCCESS)
        VK_LOG_ERR("Failed to create pipeline layout.");
}

ShaderProgram::~ShaderProgram()
{
    for(auto* setLayout : m_pSetLayouts)
    {
        m_pDevice->getDeviceTable()->vkDestroyDescriptorSetLayout(m_pDevice->getHandle(), setLayout->getHandle(),
                                                                  vkAllocator());
        delete setLayout;
    }
    m_pDevice->getDeviceTable()->vkDestroyPipelineLayout(m_pDevice->getHandle(), m_pipeLayout, vkAllocator());
}

VkShaderStageFlags ShaderProgram::getConstantShaderStage(uint32_t offset, uint32_t size) const
{
    VkShaderStageFlags stage = 0;
    size += offset;
    const auto& constant = m_combineLayout.pushConstantRange;
    stage |= constant.stageFlags;
    offset += constant.size;
    return stage;
}

void ShaderProgram::createVertexInput()
{
    if (getPipelineType() != PipelineType::Geometry && getPipelineType() != PipelineType::Mesh)
    {
        return;
    }
    uint32_t size = 0;
    aph::utils::forEachBit(m_combineLayout.attributeMask, [&](uint32_t location) {
        auto& attr = m_combineLayout.vertexAttr[location];
        m_vertexInput.attributes.push_back(
            {.location = location, .binding = 0, .format = utils::getFormatFromVk(attr.format), .offset = attr.offset});
        size += attr.size;
    });
    m_vertexInput.bindings.push_back({size});
}
}  // namespace aph::vk
