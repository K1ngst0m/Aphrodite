#pragma once

#include "math/math.h"

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
    Custom
};

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
    Vec2 m_position = {0.0f, 0.0f};
    Vec2 m_size = {0.0f, 0.0f};
    bool m_enabled = true;
};

} // namespace aph
