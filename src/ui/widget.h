#pragma once

#include "math/math.h"
#include <string>

namespace aph
{

enum class WidgetType
{
    CollapsingHeader,
    DebugTexture,
    Label,
    ColorLabel,
    HorizontalSpace,
    Separator,
    VerticalSeparator,
    Button,
    SliderFloat,
    SliderFloat2,
    SliderFloat3,
    SliderFloat4,
    SliderInt,
    SliderUint,
    RadioButton,
    Checkbox,
    OneLineCheckbox,
    CursorLocation,
    Dropdown,
    Column,
    ProgressBar,
    ColorSlider,
    Histogram,
    PlotLines,
    ColorPicker,
    Color3Picker,
    TextBox,
    DynamicText,
    FilledRect,
    DrawText,
    DrawTooltip,
    DrawLine,
    DrawCurve,
    CameraControl,
    ShaderInfo,
    TreeNode,
    Custom
};

// Convert WidgetType enum to string representation
inline std::string ToString(WidgetType type)
{
    switch (type)
    {
    case WidgetType::CollapsingHeader:
        return "CollapsingHeader";
    case WidgetType::DebugTexture:
        return "DebugTexture";
    case WidgetType::Label:
        return "Label";
    case WidgetType::ColorLabel:
        return "ColorLabel";
    case WidgetType::HorizontalSpace:
        return "HorizontalSpace";
    case WidgetType::Separator:
        return "Separator";
    case WidgetType::VerticalSeparator:
        return "VerticalSeparator";
    case WidgetType::Button:
        return "Button";
    case WidgetType::SliderFloat:
        return "SliderFloat";
    case WidgetType::SliderFloat2:
        return "SliderFloat2";
    case WidgetType::SliderFloat3:
        return "SliderFloat3";
    case WidgetType::SliderFloat4:
        return "SliderFloat4";
    case WidgetType::SliderInt:
        return "SliderInt";
    case WidgetType::SliderUint:
        return "SliderUint";
    case WidgetType::RadioButton:
        return "RadioButton";
    case WidgetType::Checkbox:
        return "Checkbox";
    case WidgetType::OneLineCheckbox:
        return "OneLineCheckbox";
    case WidgetType::CursorLocation:
        return "CursorLocation";
    case WidgetType::Dropdown:
        return "Dropdown";
    case WidgetType::Column:
        return "Column";
    case WidgetType::ProgressBar:
        return "ProgressBar";
    case WidgetType::ColorSlider:
        return "ColorSlider";
    case WidgetType::Histogram:
        return "Histogram";
    case WidgetType::PlotLines:
        return "PlotLines";
    case WidgetType::ColorPicker:
        return "ColorPicker";
    case WidgetType::Color3Picker:
        return "Color3Picker";
    case WidgetType::TextBox:
        return "TextBox";
    case WidgetType::DynamicText:
        return "DynamicText";
    case WidgetType::FilledRect:
        return "FilledRect";
    case WidgetType::DrawText:
        return "DrawText";
    case WidgetType::DrawTooltip:
        return "DrawTooltip";
    case WidgetType::DrawLine:
        return "DrawLine";
    case WidgetType::DrawCurve:
        return "DrawCurve";
    case WidgetType::CameraControl:
        return "CameraControl";
    case WidgetType::ShaderInfo:
        return "ShaderInfo";
    case WidgetType::TreeNode:
        return "TreeNode";
    case WidgetType::Custom:
        return "Custom";
    default:
        return "Unknown";
    }
}

// Forward declare UI
class UI;

// Base widget class
class Widget
{
public:
    explicit Widget(UI* pUI)
        : m_pUI(pUI)
    {
    }

    virtual ~Widget() = default;

    // Draw the widget
    virtual void draw() = 0;

    // Get the widget type
    virtual WidgetType getType() const = 0;

    // Enable/disable the widget
    void setEnabled(bool enabled)
    {
        m_enabled = enabled;
    }

    bool isEnabled() const
    {
        return m_enabled;
    }

    // Set widget position
    void setPosition(const Vec2& position)
    {
        m_position = position;
    }

    const Vec2& getPosition() const
    {
        return m_position;
    }

    // Set widget size
    void setSize(const Vec2& size)
    {
        m_size = size;
    }

    const Vec2& getSize() const
    {
        return m_size;
    }

    // Set widget label
    void setLabel(const std::string& label)
    {
        m_label = label;
    }

    const std::string& getLabel() const
    {
        return m_label;
    }

protected:
    UI* m_pUI = nullptr;
    std::string m_label;
    Vec2 m_position = { 0.0f, 0.0f };
    Vec2 m_size     = { 0.0f, 0.0f };
    bool m_enabled  = true;
};

} // namespace aph
