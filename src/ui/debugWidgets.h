#pragma once

#include "imgui.h"
#include "resource/shader/shaderAsset.h"
#include "ui.h"
#include "widget.h"
#include "widgets.h"

namespace aph
{
/**
 * ShaderInfoWidget - Displays detailed information about a shader's reflection data
 * 
 * This widget visualizes the shader reflection data in a tree structure, showing:
 * 1. Basic shader asset info
 * 2. Resource layouts (descriptor sets, bindings, etc.)
 * 3. Pipeline layout information
 * 4. Input/output attributes
 */
class ShaderInfoWidget : public Widget
{
public:
    explicit ShaderInfoWidget(UI* pUI);
    ~ShaderInfoWidget() override;

    // Set the shader asset to display
    void setShaderAsset(ShaderAsset* pShaderAsset);
    ShaderAsset* getShaderAsset() const;

    // Update the UI with current shader data
    void updateShaderInfo();

    // Widget interface
    void draw() override;
    WidgetType getType() const override;

private:
    // Create all the UI widgets
    void setupWidgets();

    // Helper functions to display different aspects of the shader info
    void updateBasicShaderInfo();
    void updateReflectionInfo();
    void updateDescriptorSets();
    void updateVertexInput();
    void updatePushConstants();
    void updateShaderStageInfo();
    void updatePipelineLayoutInfo();

    // Helper to format shader stages as a readable string
    std::string formatShaderStages(ShaderStageFlags stages);

    // Helper to format binding type as a readable string
    std::string getResourceTypeName(const ShaderLayout& layout, uint32_t binding);

private:
    // The shader asset being visualized
    ShaderAsset* m_pShaderAsset = nullptr;

    // Internal widgets for UI components
    SmallVector<Widget*> m_widgets;

    // Main container widgets
    CollapsingHeader* m_basicInfoHeader      = nullptr;
    CollapsingHeader* m_reflectionHeader     = nullptr;
    CollapsingHeader* m_pipelineLayoutHeader = nullptr;

    // Basic info widgets
    DynamicText* m_shaderNameText   = nullptr;
    DynamicText* m_shaderSourceText = nullptr;
    DynamicText* m_pipelineTypeText = nullptr;
    DynamicText* m_activeStagesText = nullptr;

    // Flag to track if we need to rebuild the dynamic UI elements
    bool m_needsRebuild = true;

    // Container for tracking dynamically created widgets
    SmallVector<Widget*> m_stageInfoWidgets;

    // Reflection data containers
    TreeNode* m_descriptorSetsNode = nullptr;
    TreeNode* m_vertexInputNode    = nullptr;
    TreeNode* m_pushConstantsNode  = nullptr;
    TreeNode* m_shaderStagesNode   = nullptr;

    // Pipeline layout container
    TreeNode* m_setLayoutsNode        = nullptr;
    TreeNode* m_vertexInputLayoutNode = nullptr;
    TreeNode* m_pushConstantRangeNode = nullptr;
};

} // namespace aph