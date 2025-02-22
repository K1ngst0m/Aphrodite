#include "pipeline.h"
#include "api/vulkan/vkInit.h"
#include "device.h"
#include "shader.h"
#include "pipelineBuilder.h"

namespace aph::vk
{
Pipeline::Pipeline(Device* pDevice, const ComputePipelineCreateInfo& createInfo, HandleType handle) :
    ResourceHandle(handle),
    m_pDevice(pDevice),
    m_pProgram(createInfo.pProgram),
    m_type(PipelineType::Compute)
{
    APH_ASSERT(m_pProgram);
}

Pipeline::Pipeline(Device* pDevice, const GraphicsPipelineCreateInfo& createInfo, HandleType handle) :
    ResourceHandle(handle),
    m_pDevice(pDevice),
    m_pProgram(createInfo.pProgram),
    m_type(createInfo.type)
{
    APH_ASSERT(m_pProgram);
}

void PipelineAllocator::setupPipelineKey(const VkPipelineBinaryKeyKHR& pipelineKey, Pipeline* pPipeline)
{
    const auto* table      = m_pDevice->getDeviceTable();
    VkDevice    device     = m_pDevice->getHandle();
    auto        vkPipeline = pPipeline->getHandle();

    m_pipelineMap[pipelineKey] = pPipeline;
    VkPipelineBinaryCreateInfoKHR pipelineBinaryCreateInfo{
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_BINARY_CREATE_INFO_KHR,
        .pNext               = NULL,
        .pKeysAndDataInfo    = NULL,
        .pipeline            = vkPipeline,
        .pPipelineCreateInfo = NULL,
    };

    VkPipelineBinaryHandlesInfoKHR handlesInfo{
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_BINARY_HANDLES_INFO_KHR,
        .pNext               = NULL,
        .pipelineBinaryCount = 0,
        .pPipelineBinaries   = NULL,
    };

    _VR(table->vkCreatePipelineBinariesKHR(device, &pipelineBinaryCreateInfo, vkAllocator(), &handlesInfo));

    std::vector<VkPipelineBinaryKHR> pipelineBinaries;
    pipelineBinaries.resize(handlesInfo.pipelineBinaryCount);

    handlesInfo.pPipelineBinaries   = pipelineBinaries.data();
    handlesInfo.pipelineBinaryCount = pipelineBinaries.size();

    _VR(table->vkCreatePipelineBinariesKHR(device, &pipelineBinaryCreateInfo, vkAllocator(), &handlesInfo));

    std::vector<VkPipelineBinaryKeyKHR> binaryKeys;
    binaryKeys.resize(handlesInfo.pipelineBinaryCount, {
                                                           .sType = VK_STRUCTURE_TYPE_PIPELINE_BINARY_KEY_KHR,
                                                           .pNext = nullptr,
                                                       });

    // Store to application cache
    for(int i = 0; i < handlesInfo.pipelineBinaryCount; ++i)
    {
        VkPipelineBinaryDataInfoKHR binaryInfo{
            .sType          = VK_STRUCTURE_TYPE_PIPELINE_BINARY_DATA_INFO_KHR,
            .pNext          = NULL,
            .pipelineBinary = pipelineBinaries[i],
        };

        size_t binaryDataSize = 0;
        _VR(table->vkGetPipelineBinaryDataKHR(device, &binaryInfo, &binaryKeys[i], &binaryDataSize, NULL));
        std::vector<uint8_t> binaryData{};
        binaryData.resize(binaryDataSize);

        _VR(table->vkGetPipelineBinaryDataKHR(device, &binaryInfo, &binaryKeys[i], &binaryDataSize, binaryData.data()));
        m_binaryKeyDataMap[binaryKeys[i]].rawData = std::move(binaryData);
        m_binaryKeyDataMap[binaryKeys[i]].binary  = pipelineBinaries[i];
    }

    m_pipelineKeyBinaryKeysMap[pipelineKey] = binaryKeys;
    VkReleaseCapturedPipelineDataInfoKHR releaseInfo{
        .sType    = VK_STRUCTURE_TYPE_RELEASE_CAPTURED_PIPELINE_DATA_INFO_KHR,
        .pNext    = nullptr,
        .pipeline = vkPipeline,
    };
    table->vkReleaseCapturedPipelineDataKHR(device, &releaseInfo, vkAllocator());
}

Pipeline* PipelineAllocator::getPipeline(const ComputePipelineCreateInfo& createInfo)
{
    const auto*    table   = m_pDevice->getDeviceTable();
    VkDevice       device  = m_pDevice->getHandle();
    ShaderProgram* program = createInfo.pProgram;
    APH_ASSERT(program);

    VkComputePipelineCreateInfo vkCreateInfo = init::computePipelineCreateInfo(program->getPipelineLayout());
    const Shader*               cs           = program->getShader(ShaderStage::CS);
    vkCreateInfo.stage =
        init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, cs->getHandle(), cs->getEntryPointName());

    // Get the pipeline key
    VkPipelineCreateInfoKHR pipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_INFO_KHR,
        .pNext = &vkCreateInfo,
    };

    VkPipelineBinaryKeyKHR pipelineKey{.sType = VK_STRUCTURE_TYPE_PIPELINE_BINARY_KEY_KHR};
    table->vkGetPipelineKeyKHR(device, &pipelineCreateInfo, &pipelineKey);

    std::lock_guard<std::mutex> holder{m_computeAcquireLock};
    if(!m_pipelineKeyBinaryKeysMap.contains(pipelineKey))
    {
        VkPipeline computePipeline = VK_NULL_HANDLE;
        _VR(m_pDevice->getDeviceTable()->vkCreateComputePipelines(m_pDevice->getHandle(), VK_NULL_HANDLE, 1,
                                                                  &vkCreateInfo, vkAllocator(), &computePipeline));
        Pipeline* pPipeline = m_pool.allocate(m_pDevice, createInfo, computePipeline);
        setupPipelineKey(pipelineKey, pPipeline);
    }

    return m_pipelineMap.at(pipelineKey);
}

Pipeline* PipelineAllocator::getPipeline(const GraphicsPipelineCreateInfo& createInfo)
{
    const auto* table  = m_pDevice->getDeviceTable();
    VkDevice    device = m_pDevice->getHandle();

    VulkanPipelineBuilder        builder{};
    VkGraphicsPipelineCreateInfo graphicsCreateInfo = builder.getCreateInfo(createInfo);

    // Get the pipeline key
    VkPipelineCreateInfoKHR pipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_INFO_KHR,
        .pNext = &graphicsCreateInfo,
    };

    VkPipelineBinaryKeyKHR pipelineKey{.sType = VK_STRUCTURE_TYPE_PIPELINE_BINARY_KEY_KHR};
    table->vkGetPipelineKeyKHR(device, &pipelineCreateInfo, &pipelineKey);

    std::lock_guard<std::mutex> holder{m_graphicsAcquireLock};
    if(!m_pipelineKeyBinaryKeysMap.contains(pipelineKey))
    {
        // Create the pipeline
        VkPipeline graphicsPipeline;
        table->vkCreateGraphicsPipelines(device, NULL, 1, &graphicsCreateInfo, vkAllocator(), &graphicsPipeline);
        Pipeline* pipeline = m_pool.allocate(m_pDevice, createInfo, graphicsPipeline);
        setupPipelineKey(pipelineKey, pipeline);
    }

    return m_pipelineMap.at(pipelineKey);
}

void PipelineAllocator::clear()
{
    auto& table  = *m_pDevice->getDeviceTable();
    auto  device = m_pDevice->getHandle();
    for(auto& [_, pPipeline] : m_pipelineMap)
    {
        table.vkDestroyPipeline(device, pPipeline->getHandle(), vkAllocator());
    }
    m_pool.clear();
    for(auto& [key, binaryData] : m_binaryKeyDataMap)
    {
        table.vkDestroyPipelineBinaryKHR(device, binaryData.binary, vkAllocator());
    }
}
}  // namespace aph::vk
