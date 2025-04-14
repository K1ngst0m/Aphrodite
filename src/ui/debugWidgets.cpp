#include "debugWidgets.h"
#include "common/profiler.h"
#include "imgui.h"
#include <format>

namespace aph
{

// Helper function to convert descriptor type to string
static const char* descriptorTypeToString(::vk::DescriptorType type)
{
    switch (type)
    {
    case ::vk::DescriptorType::eSampler:
        return "Sampler";
    case ::vk::DescriptorType::eCombinedImageSampler:
        return "Combined Image Sampler";
    case ::vk::DescriptorType::eSampledImage:
        return "Sampled Image";
    case ::vk::DescriptorType::eStorageImage:
        return "Storage Image";
    case ::vk::DescriptorType::eUniformTexelBuffer:
        return "Uniform Texel Buffer";
    case ::vk::DescriptorType::eStorageTexelBuffer:
        return "Storage Texel Buffer";
    case ::vk::DescriptorType::eUniformBuffer:
        return "Uniform Buffer";
    case ::vk::DescriptorType::eStorageBuffer:
        return "Storage Buffer";
    case ::vk::DescriptorType::eUniformBufferDynamic:
        return "Uniform Buffer Dynamic";
    case ::vk::DescriptorType::eStorageBufferDynamic:
        return "Storage Buffer Dynamic";
    case ::vk::DescriptorType::eInputAttachment:
        return "Input Attachment";
    default:
        return "Unknown";
    }
}

// Helper function to convert format to string
static const char* formatToString(Format format)
{
    switch (format)
    {
    case Format::R8_UNORM:
        return "R8_UNORM";
    case Format::R8_SNORM:
        return "R8_SNORM";
    case Format::R8_UINT:
        return "R8_UINT";
    case Format::R8_SINT:
        return "R8_SINT";
    case Format::R32_FLOAT:
        return "R32_FLOAT";
    case Format::RG32_FLOAT:
        return "RG32_FLOAT";
    case Format::RGB32_FLOAT:
        return "RGB32_FLOAT";
    case Format::RGBA32_FLOAT:
        return "RGBA32_FLOAT";
    // Add more format conversions as needed
    default:
        return "Unknown";
    }
}

ShaderInfoWidget::ShaderInfoWidget(UI* pUI)
    : Widget(pUI)
{
    setupWidgets();
}

ShaderInfoWidget::~ShaderInfoWidget()
{
    // No need to manually delete widgets as they are managed by the UI system
}

void ShaderInfoWidget::setShaderAsset(ShaderAsset* pShaderAsset)
{
    m_pShaderAsset = pShaderAsset;
    m_needsRebuild = true;
    updateShaderInfo();
}

ShaderAsset* ShaderInfoWidget::getShaderAsset() const
{
    return m_pShaderAsset;
}

void ShaderInfoWidget::updateShaderInfo()
{
    if (!m_pShaderAsset || !m_pShaderAsset->isValid())
    {
        // Update widgets to show no valid shader
        if (m_shaderNameText)
        {
            m_shaderNameText->setText("No shader selected");
        }
        if (m_shaderSourceText)
        {
            m_shaderSourceText->setText("");
        }
        if (m_pipelineTypeText)
        {
            m_pipelineTypeText->setText("");
        }
        if (m_activeStagesText)
        {
            m_activeStagesText->setText("");
        }

        // Clear any dynamic content
        m_stageInfoWidgets.clear();

        return;
    }

    // Update basic shader info
    updateBasicShaderInfo();

    // If we need to rebuild, update all sections
    if (m_needsRebuild)
    {
        // Clear previous dynamic widgets
        m_stageInfoWidgets.clear();

        // Update all sections
        updateReflectionInfo();
        updatePipelineLayoutInfo();

        m_needsRebuild = false;
    }
}

void ShaderInfoWidget::draw()
{
    APH_PROFILER_SCOPE();

    if (!m_enabled)
    {
        return;
    }

    // If no valid shader, display error message
    if (!m_pShaderAsset || !m_pShaderAsset->isValid())
    {
        auto* errorText = m_pUI->createWidget<ColorLabel>();
        errorText->setLabel("No valid shader asset selected");
        errorText->setColor({ 1.0f, 0.0f, 0.0f, 1.0f });
        errorText->draw();
        return;
    }

    // Draw all widgets
    for (auto* widget : m_widgets)
    {
        if (widget && widget->isEnabled())
        {
            widget->draw();
        }
    }

    // Draw all dynamic stage info widgets
    for (auto* widget : m_stageInfoWidgets)
    {
        if (widget && widget->isEnabled())
        {
            widget->draw();
        }
    }
}

WidgetType ShaderInfoWidget::getType() const
{
    return WidgetType::ShaderInfo;
}

void ShaderInfoWidget::setupWidgets()
{
    if (!m_pUI)
    {
        return;
    }

    // Create collapsing headers for main sections
    m_basicInfoHeader = m_pUI->createWidget<CollapsingHeader>();
    m_basicInfoHeader->setLabel("Shader Basic Info");
    m_basicInfoHeader->setFlags(ImGuiTreeNodeFlags_DefaultOpen);
    m_widgets.push_back(m_basicInfoHeader);

    // Create text widgets for basic shader properties
    m_shaderNameText = m_pUI->createWidget<DynamicText>();
    m_shaderNameText->setLabel("Shader Name");
    m_basicInfoHeader->addWidget(m_shaderNameText);

    m_shaderSourceText = m_pUI->createWidget<DynamicText>();
    m_shaderSourceText->setLabel("Source");
    m_basicInfoHeader->addWidget(m_shaderSourceText);

    m_pipelineTypeText = m_pUI->createWidget<DynamicText>();
    m_pipelineTypeText->setLabel("Pipeline Type");
    m_basicInfoHeader->addWidget(m_pipelineTypeText);

    // Add label for active shader stages
    m_activeStagesText = m_pUI->createWidget<DynamicText>();
    m_activeStagesText->setLabel("Active Shader Stages");
    m_basicInfoHeader->addWidget(m_activeStagesText);

    // Create collapsing headers for the main data sections
    m_reflectionHeader = m_pUI->createWidget<CollapsingHeader>();
    m_reflectionHeader->setLabel("Reflection Data");
    m_widgets.push_back(m_reflectionHeader);

    // Pipeline layout section
    m_pipelineLayoutHeader = m_pUI->createWidget<CollapsingHeader>();
    m_pipelineLayoutHeader->setLabel("Pipeline Layout");
    m_widgets.push_back(m_pipelineLayoutHeader);

    // The tree nodes will be created dynamically in updateShaderInfo
}

void ShaderInfoWidget::updateBasicShaderInfo()
{
    if (!m_pShaderAsset)
    {
        return;
    }

    // Update basic text fields
    m_shaderNameText->setText(m_pShaderAsset->getDebugName());
    m_shaderSourceText->setText(m_pShaderAsset->getSourceDesc());
    m_pipelineTypeText->setText(m_pShaderAsset->getPipelineTypeString());

    // First, clear any widgets that were previously in basicInfoHeader but not part of the fixed UI
    size_t fixedWidgetCount = 4; // The 4 text fields we manually created
    while (m_basicInfoHeader->getWidgetCount() > fixedWidgetCount)
    {
        m_basicInfoHeader->removeWidget(fixedWidgetCount);
    }

    // Clear existing stage info widgets vector, but don't try to delete them
    m_stageInfoWidgets.clear();

    // Add active shader stages
    static constexpr std::array stages = { ShaderStage::VS, ShaderStage::TCS, ShaderStage::TES, ShaderStage::GS,
                                           ShaderStage::FS, ShaderStage::CS,  ShaderStage::TS,  ShaderStage::MS };

    // Create a string to display all stages
    std::string activeStages;

    for (const auto& stage : stages)
    {
        if (m_pShaderAsset->getShader(stage))
        {
            if (!activeStages.empty())
            {
                activeStages += ", ";
            }
            activeStages += vk::utils::toString(stage);
        }
    }

    // Update the active stages text
    m_activeStagesText->setText(activeStages);
}

void ShaderInfoWidget::updateReflectionInfo()
{
    // First, clear the parent header by removing all widgets
    while (m_reflectionHeader->getWidgetCount() > 0)
    {
        m_reflectionHeader->removeWidget(0);
    }

    // We need to recreate the contents each time for each subsection
    // rather than trying to clear the existing widgets

    // We'll create new tree nodes for each section
    m_descriptorSetsNode = m_pUI->createWidget<TreeNode>();
    m_descriptorSetsNode->setLabel("Descriptor Sets");
    m_descriptorSetsNode->setFlags(ImGuiTreeNodeFlags_DefaultOpen);

    m_vertexInputNode = m_pUI->createWidget<TreeNode>();
    m_vertexInputNode->setLabel("Vertex Input");
    m_vertexInputNode->setFlags(ImGuiTreeNodeFlags_DefaultOpen);

    m_pushConstantsNode = m_pUI->createWidget<TreeNode>();
    m_pushConstantsNode->setLabel("Push Constants");
    m_pushConstantsNode->setFlags(ImGuiTreeNodeFlags_DefaultOpen);

    m_shaderStagesNode = m_pUI->createWidget<TreeNode>();
    m_shaderStagesNode->setLabel("Shader Stage Info");
    m_shaderStagesNode->setFlags(ImGuiTreeNodeFlags_DefaultOpen);

    // Add our new nodes to the reflection header
    m_reflectionHeader->addWidget(m_descriptorSetsNode);
    m_reflectionHeader->addWidget(m_vertexInputNode);
    m_reflectionHeader->addWidget(m_pushConstantsNode);
    m_reflectionHeader->addWidget(m_shaderStagesNode);

    // Update all reflection info subsections
    updateDescriptorSets();
    updateVertexInput();
    updatePushConstants();
    updateShaderStageInfo();
}

void ShaderInfoWidget::updateDescriptorSets()
{
    if (!m_pShaderAsset || !m_descriptorSetsNode)
    {
        return;
    }

    // First, remove any existing widgets from the node
    // Note: TreeNode doesn't support clearing widgets with addWidget(nullptr)
    // We'll create new widgets each time instead

    const auto& reflectionData = m_pShaderAsset->getReflectionData();
    const auto& resourceLayout = reflectionData.resourceLayout;

    // Get active descriptor sets
    auto activeSets = ShaderReflector::getActiveDescriptorSets(reflectionData);
    if (activeSets.empty())
    {
        auto* noSetsText = m_pUI->createWidget<ColorLabel>();
        noSetsText->setLabel("No descriptor sets used");
        noSetsText->setColor({ 1.0f, 0.7f, 0.0f, 1.0f });
        m_descriptorSetsNode->addWidget(noSetsText);
        return;
    }

    // Display descriptor set information
    for (auto setIndex : activeSets)
    {
        // Create a tree node for each descriptor set
        auto* setNode = m_pUI->createWidget<TreeNode>();
        setNode->setLabel(std::format("Set {}", setIndex));
        m_descriptorSetsNode->addWidget(setNode);

        const auto& setInfo       = resourceLayout.setInfos[setIndex];
        const auto& descResources = reflectionData.descriptorResources[setIndex];

        // Check if this is a bindless set
        bool isBindless = resourceLayout.bindlessDescriptorSetMask.test(setIndex);
        if (isBindless)
        {
            auto* bindlessText = m_pUI->createWidget<ColorLabel>();
            bindlessText->setLabel("Bindless Descriptor Set");
            bindlessText->setColor({ 0.0f, 1.0f, 0.5f, 1.0f });
            setNode->addWidget(bindlessText);
        }

        // Add shader stages info
        auto* stagesText = m_pUI->createWidget<DynamicText>();
        stagesText->setLabel("Shader Stages");
        stagesText->setText(formatShaderStages(setInfo.stagesForSets));
        setNode->addWidget(stagesText);

        // Display bindings
        auto* bindingsNode = m_pUI->createWidget<TreeNode>();
        bindingsNode->setLabel("Bindings");
        setNode->addWidget(bindingsNode);

        for (const auto& binding : descResources.bindings)
        {
            uint32_t bindingIndex = binding.binding;

            // Determine if this binding is an array
            uint32_t arraySize  = setInfo.shaderLayout.arraySize[bindingIndex];
            bool isUnsizedArray = (arraySize == ShaderLayout::UNSIZED_ARRAY);

            std::string arrayDesc;
            if (isUnsizedArray)
            {
                arrayDesc = "(Unsized Array)";
            }
            else if (arraySize > 1)
            {
                arrayDesc = std::format("(Array[{}])", arraySize);
            }

            // Create binding info text
            auto* bindingText = m_pUI->createWidget<DynamicText>();
            bindingText->setLabel(std::format("Binding {}", bindingIndex));
            bindingText->setText(std::format("{} {}", descriptorTypeToString(binding.descriptorType), arrayDesc));
            bindingsNode->addWidget(bindingText);

            // Add binding details
            auto* countText = m_pUI->createWidget<DynamicText>();
            countText->setLabel("Count");
            countText->setText(std::format("{}", binding.descriptorCount));
            bindingsNode->addWidget(countText);

            auto* bindingStagesText = m_pUI->createWidget<DynamicText>();
            bindingStagesText->setLabel("Stages");
            bindingStagesText->setText(formatShaderStages(aph::vk::utils::getShaderStages(binding.stageFlags)));
            bindingsNode->addWidget(bindingStagesText);
        }

        // Display pool sizes
        auto* poolSizesNode = m_pUI->createWidget<TreeNode>();
        poolSizesNode->setLabel("Pool Sizes");
        setNode->addWidget(poolSizesNode);

        for (const auto& poolSize : descResources.poolSizes)
        {
            auto* poolSizeText = m_pUI->createWidget<DynamicText>();
            poolSizeText->setLabel(descriptorTypeToString(poolSize.type));
            poolSizeText->setText(std::format("{}", poolSize.descriptorCount));
            poolSizesNode->addWidget(poolSizeText);
        }
    }
}

void ShaderInfoWidget::updateVertexInput()
{
    if (!m_pShaderAsset || !m_vertexInputNode)
    {
        return;
    }

    // No need to clear existing widgets - we'll recreate everything

    const auto& reflectionData = m_pShaderAsset->getReflectionData();
    const auto& vertexInput    = reflectionData.vertexInput;

    if (vertexInput.attributes.empty())
    {
        auto* noInputText = m_pUI->createWidget<ColorLabel>();
        noInputText->setLabel("No vertex input attributes");
        noInputText->setColor({ 1.0f, 0.7f, 0.0f, 1.0f });
        m_vertexInputNode->addWidget(noInputText);
        return;
    }

    // Display vertex bindings
    if (!vertexInput.bindings.empty())
    {
        auto* bindingsNode = m_pUI->createWidget<TreeNode>();
        bindingsNode->setLabel("Bindings");
        m_vertexInputNode->addWidget(bindingsNode);

        for (size_t i = 0; i < vertexInput.bindings.size(); i++)
        {
            const auto& binding = vertexInput.bindings[i];

            auto* bindingText = m_pUI->createWidget<DynamicText>();
            bindingText->setLabel(std::format("Binding {}", i));
            bindingText->setText(std::format("Stride: {} bytes", binding.stride));
            bindingsNode->addWidget(bindingText);
        }
    }

    // Display vertex attributes
    if (!vertexInput.attributes.empty())
    {
        auto* attributesNode = m_pUI->createWidget<TreeNode>();
        attributesNode->setLabel("Attributes");
        m_vertexInputNode->addWidget(attributesNode);

        for (const auto& attr : vertexInput.attributes)
        {
            auto* attrNode = m_pUI->createWidget<TreeNode>();
            attrNode->setLabel(std::format("Location {}", attr.location));
            attributesNode->addWidget(attrNode);

            auto* bindingText = m_pUI->createWidget<DynamicText>();
            bindingText->setLabel("Binding");
            bindingText->setText(std::format("{}", attr.binding));
            attrNode->addWidget(bindingText);

            auto* formatText = m_pUI->createWidget<DynamicText>();
            formatText->setLabel("Format");
            formatText->setText(formatToString(attr.format));
            attrNode->addWidget(formatText);

            auto* offsetText = m_pUI->createWidget<DynamicText>();
            offsetText->setLabel("Offset");
            offsetText->setText(std::format("{} bytes", attr.offset));
            attrNode->addWidget(offsetText);
        }
    }
}

void ShaderInfoWidget::updatePushConstants()
{
    if (!m_pShaderAsset || !m_pushConstantsNode)
    {
        return;
    }

    // No need to clear existing widgets

    const auto& reflectionData = m_pShaderAsset->getReflectionData();
    const auto& pushConstants  = reflectionData.pushConstantRange;

    if (pushConstants.size == 0)
    {
        auto* noPushConstantsText = m_pUI->createWidget<ColorLabel>();
        noPushConstantsText->setLabel("No push constants used");
        noPushConstantsText->setColor({ 1.0f, 0.7f, 0.0f, 1.0f });
        m_pushConstantsNode->addWidget(noPushConstantsText);
        return;
    }

    // Display push constant information
    auto* sizeText = m_pUI->createWidget<DynamicText>();
    sizeText->setLabel("Size");
    sizeText->setText(std::format("{} bytes", pushConstants.size));
    m_pushConstantsNode->addWidget(sizeText);

    auto* offsetText = m_pUI->createWidget<DynamicText>();
    offsetText->setLabel("Offset");
    offsetText->setText(std::format("{} bytes", pushConstants.offset));
    m_pushConstantsNode->addWidget(offsetText);

    auto* stagesText = m_pUI->createWidget<DynamicText>();
    stagesText->setLabel("Shader Stages");
    stagesText->setText(formatShaderStages(pushConstants.stageFlags));
    m_pushConstantsNode->addWidget(stagesText);
}

void ShaderInfoWidget::updateShaderStageInfo()
{
    if (!m_pShaderAsset || !m_shaderStagesNode)
    {
        return;
    }

    // No need to clear existing widgets

    const auto& reflectionData = m_pShaderAsset->getReflectionData();
    const auto& resourceLayout = reflectionData.resourceLayout;

    // Display specialization constants by shader stage
    if (!resourceLayout.combinedSpecConstantMask.none())
    {
        auto* specConstantsNode = m_pUI->createWidget<TreeNode>();
        specConstantsNode->setLabel("Specialization Constants");
        m_shaderStagesNode->addWidget(specConstantsNode);

        for (const auto& [stage, mask] : resourceLayout.specConstantMask)
        {
            if (!mask.none())
            {
                auto* stageNode = m_pUI->createWidget<TreeNode>();
                stageNode->setLabel(std::format("{} Stage", vk::utils::toString(stage)));
                specConstantsNode->addWidget(stageNode);

                for (size_t i = 0; i < VULKAN_NUM_TOTAL_SPEC_CONSTANTS; i++)
                {
                    if (mask.test(i))
                    {
                        auto* constantText = m_pUI->createWidget<DynamicText>();
                        constantText->setText(std::format("Constant ID {}", i));
                        stageNode->addWidget(constantText);
                    }
                }
            }
        }
    }

    static constexpr std::array stages = { ShaderStage::VS, ShaderStage::TCS, ShaderStage::TES, ShaderStage::GS,
                                           ShaderStage::FS, ShaderStage::CS,  ShaderStage::TS,  ShaderStage::MS };

    for (const auto& stage : stages)
    {
        // Don't need to store the shader ptr if we're just checking existence
        if (m_pShaderAsset->getShader(stage))
        {
            auto* stageNode = m_pUI->createWidget<TreeNode>();
            stageNode->setLabel(std::format("{} Shader", vk::utils::toString(stage)));
            m_shaderStagesNode->addWidget(stageNode);

            auto* resourceTitle = m_pUI->createWidget<Label>();
            resourceTitle->setLabel("Resource Usage:");
            stageNode->addWidget(resourceTitle);

            // Display descriptor sets used by this stage
            auto* setsLabel = m_pUI->createWidget<Label>();
            setsLabel->setLabel("Descriptor Sets:");
            stageNode->addWidget(setsLabel);

            auto activeSets = ShaderReflector::getActiveDescriptorSets(reflectionData);
            for (auto setIndex : activeSets)
            {
                const auto& setInfo = resourceLayout.setInfos[setIndex];
                if (setInfo.stagesForSets & stage)
                {
                    auto* setNode = m_pUI->createWidget<TreeNode>();
                    setNode->setLabel(std::format("Set {}", setIndex));
                    stageNode->addWidget(setNode);

                    // List bindings used by this stage
                    for (uint32_t binding = 0; binding < VULKAN_NUM_BINDINGS; binding++)
                    {
                        if (setInfo.stagesForBindings[binding] & stage)
                        {
                            auto* bindingText = m_pUI->createWidget<DynamicText>();
                            bindingText->setText(std::format("Binding {}", binding));
                            setNode->addWidget(bindingText);
                        }
                    }
                }
            }

            // Display whether this stage uses push constants
            if (reflectionData.pushConstantRange.stageFlags & stage)
            {
                auto* pushConstText = m_pUI->createWidget<DynamicText>();
                pushConstText->setText("Uses Push Constants");
                stageNode->addWidget(pushConstText);
            }
        }
    }
}

void ShaderInfoWidget::updatePipelineLayoutInfo()
{
    if (!m_pShaderAsset)
    {
        return;
    }

    // First, clear the parent header by removing all widgets
    while (m_pipelineLayoutHeader->getWidgetCount() > 0)
    {
        m_pipelineLayoutHeader->removeWidget(0);
    }

    // Create new tree nodes for pipeline layout subsections
    m_setLayoutsNode = m_pUI->createWidget<TreeNode>();
    m_setLayoutsNode->setLabel("Descriptor Set Layouts");
    m_setLayoutsNode->setFlags(ImGuiTreeNodeFlags_DefaultOpen);

    m_vertexInputLayoutNode = m_pUI->createWidget<TreeNode>();
    m_vertexInputLayoutNode->setLabel("Vertex Input Layout");
    m_vertexInputLayoutNode->setFlags(ImGuiTreeNodeFlags_DefaultOpen);

    m_pushConstantRangeNode = m_pUI->createWidget<TreeNode>();
    m_pushConstantRangeNode->setLabel("Push Constant Range");
    m_pushConstantRangeNode->setFlags(ImGuiTreeNodeFlags_DefaultOpen);

    // Add these nodes to the pipeline layout header
    m_pipelineLayoutHeader->addWidget(m_setLayoutsNode);
    m_pipelineLayoutHeader->addWidget(m_vertexInputLayoutNode);
    m_pipelineLayoutHeader->addWidget(m_pushConstantRangeNode);

    auto* pipelineLayout = m_pShaderAsset->getPipelineLayout();
    if (!pipelineLayout)
    {
        auto* noLayoutText = m_pUI->createWidget<ColorLabel>();
        noLayoutText->setLabel("No pipeline layout available");
        noLayoutText->setColor({ 1.0f, 0.3f, 0.3f, 1.0f });
        m_setLayoutsNode->addWidget(noLayoutText);
        return;
    }

    // Display active descriptor set layouts
    const auto& reflectionData = m_pShaderAsset->getReflectionData();
    auto activeSets            = ShaderReflector::getActiveDescriptorSets(reflectionData);

    auto* activeSetsText = m_pUI->createWidget<DynamicText>();
    activeSetsText->setText(std::format("Active Descriptor Set Layouts: {}", activeSets.size()));
    m_setLayoutsNode->addWidget(activeSetsText);

    // Display each set layout
    for (auto setIndex : activeSets)
    {
        auto* setNode = m_pUI->createWidget<TreeNode>();
        setNode->setLabel(std::format("Set {}", setIndex));
        m_setLayoutsNode->addWidget(setNode);

        // We don't need to store the setLayout if we're not using it
        bool isBindless = ShaderReflector::isBindlessSet(reflectionData, setIndex);

        if (isBindless)
        {
            auto* bindlessText = m_pUI->createWidget<ColorLabel>();
            bindlessText->setLabel("Bindless Descriptor Set");
            bindlessText->setColor({ 0.0f, 1.0f, 0.5f, 1.0f });
            setNode->addWidget(bindlessText);
        }

        const auto& setInfo = reflectionData.resourceLayout.setInfos[setIndex];
        auto* setStagesText = m_pUI->createWidget<DynamicText>();
        setStagesText->setLabel("Shader Stages");
        setStagesText->setText(formatShaderStages(setInfo.stagesForSets));
        setNode->addWidget(setStagesText);

        // Display bindings
        auto* bindingsNode = m_pUI->createWidget<TreeNode>();
        bindingsNode->setLabel("Bindings");
        setNode->addWidget(bindingsNode);

        const auto& bindings = reflectionData.descriptorResources[setIndex].bindings;
        for (const auto& binding : bindings)
        {
            auto* bindingText = m_pUI->createWidget<DynamicText>();
            bindingText->setText(std::format("Binding {}: {} (Count: {}, Stages: {})", binding.binding,
                                             descriptorTypeToString(binding.descriptorType), binding.descriptorCount,
                                             formatShaderStages(aph::vk::utils::getShaderStages(binding.stageFlags))));
            bindingsNode->addWidget(bindingText);
        }
    }

    // Display vertex input layout
    const auto& vertexInput = m_pShaderAsset->getVertexInput();

    auto* bindingsCountText = m_pUI->createWidget<DynamicText>();
    bindingsCountText->setText(std::format("Vertex Bindings: {}", vertexInput.bindings.size()));
    m_vertexInputLayoutNode->addWidget(bindingsCountText);

    auto* attributesCountText = m_pUI->createWidget<DynamicText>();
    attributesCountText->setText(std::format("Vertex Attributes: {}", vertexInput.attributes.size()));
    m_vertexInputLayoutNode->addWidget(attributesCountText);

    if (!vertexInput.bindings.empty())
    {
        auto* bindingsNode = m_pUI->createWidget<TreeNode>();
        bindingsNode->setLabel("Bindings");
        m_vertexInputLayoutNode->addWidget(bindingsNode);

        for (size_t i = 0; i < vertexInput.bindings.size(); i++)
        {
            const auto& binding = vertexInput.bindings[i];
            auto* bindingText   = m_pUI->createWidget<DynamicText>();
            bindingText->setText(std::format("Binding {}: Stride: {}", i, binding.stride));
            bindingsNode->addWidget(bindingText);
        }
    }

    if (!vertexInput.attributes.empty())
    {
        auto* attributesNode = m_pUI->createWidget<TreeNode>();
        attributesNode->setLabel("Attributes");
        m_vertexInputLayoutNode->addWidget(attributesNode);

        for (const auto& attr : vertexInput.attributes)
        {
            auto* attrText = m_pUI->createWidget<DynamicText>();
            attrText->setText(std::format("Location {}: Binding {}, Format: {}, Offset: {}", attr.location,
                                          attr.binding, formatToString(attr.format), attr.offset));
            attributesNode->addWidget(attrText);
        }
    }

    // Display push constant range
    const auto& pushConstantRange = m_pShaderAsset->getPushConstantRange();
    if (pushConstantRange.size > 0)
    {
        auto* offsetText = m_pUI->createWidget<DynamicText>();
        offsetText->setLabel("Offset");
        offsetText->setText(std::format("{}", pushConstantRange.offset));
        m_pushConstantRangeNode->addWidget(offsetText);

        auto* sizeText = m_pUI->createWidget<DynamicText>();
        sizeText->setLabel("Size");
        sizeText->setText(std::format("{} bytes", pushConstantRange.size));
        m_pushConstantRangeNode->addWidget(sizeText);

        auto* stagesText = m_pUI->createWidget<DynamicText>();
        stagesText->setLabel("Stages");
        stagesText->setText(formatShaderStages(pushConstantRange.stageFlags));
        m_pushConstantRangeNode->addWidget(stagesText);
    }
    else
    {
        auto* noPushConstantsText = m_pUI->createWidget<DynamicText>();
        noPushConstantsText->setText("No Push Constants");
        m_pushConstantRangeNode->addWidget(noPushConstantsText);
    }
}

std::string ShaderInfoWidget::formatShaderStages(ShaderStageFlags stages)
{
    if (stages == 0)
    {
        return "None";
    }

    std::string result;

    if (stages & ShaderStage::VS)
    {
        result += "VS ";
    }
    if (stages & ShaderStage::TCS)
    {
        result += "TCS ";
    }
    if (stages & ShaderStage::TES)
    {
        result += "TES ";
    }
    if (stages & ShaderStage::GS)
    {
        result += "GS ";
    }
    if (stages & ShaderStage::FS)
    {
        result += "FS ";
    }
    if (stages & ShaderStage::CS)
    {
        result += "CS ";
    }
    if (stages & ShaderStage::TS)
    {
        result += "TS ";
    }
    if (stages & ShaderStage::MS)
    {
        result += "MS ";
    }

    if (!result.empty() && result.back() == ' ')
    {
        result.pop_back();
    }

    return result;
}

std::string ShaderInfoWidget::getResourceTypeName(const ShaderLayout& layout, uint32_t binding)
{
    if (layout.sampledImageMask.test(binding))
    {
        return "Sampled Image";
    }
    if (layout.storageImageMask.test(binding))
    {
        return "Storage Image";
    }
    if (layout.uniformBufferMask.test(binding))
    {
        return "Uniform Buffer";
    }
    if (layout.storageBufferMask.test(binding))
    {
        return "Storage Buffer";
    }
    if (layout.sampledTexelBufferMask.test(binding))
    {
        return "Sampled Texel Buffer";
    }
    if (layout.storageTexelBufferMask.test(binding))
    {
        return "Storage Texel Buffer";
    }
    if (layout.inputAttachmentMask.test(binding))
    {
        return "Input Attachment";
    }
    if (layout.samplerMask.test(binding))
    {
        return "Sampler";
    }
    if (layout.separateImageMask.test(binding))
    {
        return "Separate Image";
    }

    return "Unknown";
}

} // namespace aph
