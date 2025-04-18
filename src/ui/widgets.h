#pragma once

#include "imgui.h"
#include "ui.h"
#include "widget.h"

//
// Container Widgets
//
namespace aph
{
// Container type enum for identifying container types without RTTI
enum class ContainerType
{
    Generic,
    Window
};

// Convert ContainerType enum to string representation
inline std::string ToString(ContainerType type)
{
    switch (type)
    {
    case ContainerType::Generic:
        return "Generic";
    case ContainerType::Window:
        return "Window";
    default:
        return "Unknown";
    }
}

// Container widget for managing groups of widgets
class WidgetContainer
{
public:
    explicit WidgetContainer(UI* pUI);
    virtual ~WidgetContainer() = default;

    // Get container type
    virtual ContainerType getType() const;

    // Add an existing widget to this container
    template <typename T>
    void addWidget(T* widget)
    {
        APH_ASSERT(widget);
        m_widgets.push_back(static_cast<Widget*>(widget));
    }

    // Draw all widgets in the container
    void drawAll();

    // Clear all widgets
    void clear();

    // Enable/disable all widgets
    void setAllEnabled(bool enabled);

    // Get number of widgets
    size_t size() const;

protected:
    UI* m_pUI = nullptr;
    SmallVector<Widget*> m_widgets;

    // For breadcrumb tracking
    uint32_t m_breadcrumbId = UINT32_MAX;

    // Allow UI to access the breadcrumbId
    friend class UI;
};

// A window that contains widgets
class WidgetWindow : public WidgetContainer
{
public:
    explicit WidgetWindow(UI* pUI);

    // Override to identify as window type
    ContainerType getType() const override;

    void setTitle(const std::string& title);
    const std::string& getTitle() const;

    void setSize(const Vec2& size);
    const Vec2& getSize() const;

    void setPosition(const Vec2& position);
    const Vec2& getPosition() const;

    void setFlags(ImGuiWindowFlags flags);
    ImGuiWindowFlags getFlags() const;

    bool begin();
    void end();

    bool isOpen() const;
    void setOpen(bool open);

    void draw();

private:
    std::string m_title      = "Widget Window";
    Vec2 m_size              = { 0.0f, 0.0f };
    Vec2 m_position          = { 0.0f, 0.0f };
    ImGuiWindowFlags m_flags = 0;
    bool m_open              = true;
};
} // namespace aph

//
// Common Widgets
//
namespace aph
{
// Label widget
class Label : public Widget
{
public:
    explicit Label(UI* pUI);
    void draw() override;
    WidgetType getType() const override;
};

// Color Label widget
class ColorLabel : public Widget
{
public:
    explicit ColorLabel(UI* pUI);
    void setColor(const Vec4& color);
    const Vec4& getColor() const;
    void draw() override;
    WidgetType getType() const override;

private:
    Vec4 m_color = { 1.0f, 1.0f, 1.0f, 1.0f };
};

// Button widget
class Button : public Widget
{
public:
    explicit Button(UI* pUI);
    void setCallback(std::function<void()> callback);
    void draw() override;
    WidgetType getType() const override;

private:
    std::function<void()> m_callback;
};

// Checkbox widget
class Checkbox : public Widget
{
public:
    explicit Checkbox(UI* pUI);
    void setValue(bool value);
    bool getValue() const;
    void setCallback(std::function<void(bool)> callback);
    void draw() override;
    WidgetType getType() const override;

private:
    bool m_value = false;
    std::function<void(bool)> m_callback;
};

// Slider Float widget
class SliderFloat : public Widget
{
public:
    explicit SliderFloat(UI* pUI);
    void setValue(float value);
    float getValue() const;
    void setRange(float min, float max);
    void setFormat(const std::string& format);
    void setCallback(std::function<void(float)> callback);
    void draw() override;
    WidgetType getType() const override;

private:
    float m_value        = 0.0f;
    float m_min          = 0.0f;
    float m_max          = 1.0f;
    std::string m_format = "%.3f";
    std::function<void(float)> m_callback;
};

// Slider Float2 widget
class SliderFloat2 : public Widget
{
public:
    explicit SliderFloat2(UI* pUI);
    void setValue(const Vec2& value);
    const Vec2& getValue() const;
    void setRange(float min, float max);
    void setFormat(const std::string& format);
    void setCallback(std::function<void(const Vec2&)> callback);
    void draw() override;
    WidgetType getType() const override;

private:
    Vec2 m_value         = { 0.0f, 0.0f };
    float m_min          = 0.0f;
    float m_max          = 1.0f;
    std::string m_format = "%.3f";
    std::function<void(const Vec2&)> m_callback;
};

// Dropdown widget
class Dropdown : public Widget
{
public:
    explicit Dropdown(UI* pUI);
    void setOptions(const SmallVector<std::string>& options);
    void setSelectedIndex(int index);
    int getSelectedIndex() const;
    const std::string& getSelectedOption() const;
    void setCallback(std::function<void(int, const std::string&)> callback);
    void draw() override;
    WidgetType getType() const override;

private:
    SmallVector<std::string> m_options;
    int m_selectedIndex = 0;
    std::function<void(int, const std::string&)> m_callback;
};

// Collapsing Header widget
class CollapsingHeader : public Widget
{
public:
    explicit CollapsingHeader(UI* pUI);
    void addWidget(Widget* widget);
    void removeWidget(size_t index);

    size_t getWidgetCount() const;

    void draw() override;
    void setFlags(ImGuiTreeNodeFlags flags);
    WidgetType getType() const override;

private:
    SmallVector<Widget*> m_widgets;
    ImGuiTreeNodeFlags m_flags = 0;
};

// Separator widget
class Separator : public Widget
{
public:
    explicit Separator(UI* pUI);
    void draw() override;
    WidgetType getType() const override;
};

// Progress Bar widget
class ProgressBar : public Widget
{
public:
    explicit ProgressBar(UI* pUI);
    void setValue(float value);
    float getValue() const;
    void draw() override;
    WidgetType getType() const override;

private:
    float m_value = 0.0f;
};

// Custom widget for implementing specialized behavior
class CustomWidget : public Widget
{
public:
    explicit CustomWidget(UI* pUI);
    void setDrawCallback(std::function<void()> callback);
    void draw() override;
    WidgetType getType() const override;

private:
    std::function<void()> m_drawCallback;
};

} // namespace aph

//
// Composite Widgets
//
namespace aph
{
// Color Picker widget
class ColorPicker : public Widget
{
public:
    explicit ColorPicker(UI* pUI);
    void setColor(const Vec4& color);
    const Vec4& getColor() const;
    void setFlags(ImGuiColorEditFlags flags);
    void setCallback(std::function<void(const Vec4&)> callback);
    void draw() override;
    WidgetType getType() const override;

private:
    Vec4 m_color                = { 1.0f, 1.0f, 1.0f, 1.0f };
    ImGuiColorEditFlags m_flags = 0;
    std::function<void(const Vec4&)> m_callback;
};

// Color3 Picker widget (RGB only)
class Color3Picker : public Widget
{
public:
    explicit Color3Picker(UI* pUI);
    void setColor(const Vec3& color);
    const Vec3& getColor() const;
    void setFlags(ImGuiColorEditFlags flags);
    void setCallback(std::function<void(const Vec3&)> callback);
    void draw() override;
    WidgetType getType() const override;

private:
    Vec3 m_color                = { 1.0f, 1.0f, 1.0f };
    ImGuiColorEditFlags m_flags = 0;
    std::function<void(const Vec3&)> m_callback;
};

// Plot Lines widget
class PlotLines : public Widget
{
public:
    explicit PlotLines(UI* pUI);
    void setValues(const SmallVector<float>& values);
    const SmallVector<float>& getValues() const;
    void setScaleMin(float min);
    void setScaleMax(float max);
    void setOverlayText(const std::string& text);
    void draw() override;
    WidgetType getType() const override;

private:
    SmallVector<float> m_values;
    float m_scaleMin = FLT_MAX;
    float m_scaleMax = FLT_MAX;
    std::string m_overlayText;
};

// Histogram widget
class Histogram : public Widget
{
public:
    explicit Histogram(UI* pUI);
    void setValues(const SmallVector<float>& values);
    const SmallVector<float>& getValues() const;
    void setScaleMin(float min);
    void setScaleMax(float max);
    void setOverlayText(const std::string& text);
    void draw() override;
    WidgetType getType() const override;

private:
    SmallVector<float> m_values;
    float m_scaleMin = FLT_MAX;
    float m_scaleMax = FLT_MAX;
    std::string m_overlayText;
};

// TextBox widget
class TextBox : public Widget
{
public:
    explicit TextBox(UI* pUI);
    void setText(const std::string& text);
    const std::string& getText() const;
    void setBufferSize(size_t size);
    void setFlags(ImGuiInputTextFlags flags);
    void setCallback(std::function<void(const std::string&)> callback);
    void setMultiline(bool multiline);
    void draw() override;
    WidgetType getType() const override;

private:
    std::string m_text;
    SmallVector<char> m_buffer  = SmallVector<char>(256);
    size_t m_bufferExtraSize    = 256;
    ImGuiInputTextFlags m_flags = 0;
    bool m_multiline            = false;
    std::function<void(const std::string&)> m_callback;
};

// Slider Float3 widget
class SliderFloat3 : public Widget
{
public:
    explicit SliderFloat3(UI* pUI);
    void setValue(const Vec3& value);
    const Vec3& getValue() const;
    void setRange(float min, float max);
    void setFormat(const std::string& format);
    void setCallback(std::function<void(const Vec3&)> callback);
    void draw() override;
    WidgetType getType() const override;

private:
    Vec3 m_value         = { 0.0f, 0.0f, 0.0f };
    float m_min          = 0.0f;
    float m_max          = 1.0f;
    std::string m_format = "%.3f";
    std::function<void(const Vec3&)> m_callback;
};

// Slider Float4 widget
class SliderFloat4 : public Widget
{
public:
    explicit SliderFloat4(UI* pUI);
    void setValue(const Vec4& value);
    const Vec4& getValue() const;
    void setRange(float min, float max);
    void setFormat(const std::string& format);
    void setCallback(std::function<void(const Vec4&)> callback);
    void draw() override;
    WidgetType getType() const override;

private:
    Vec4 m_value         = { 0.0f, 0.0f, 0.0f, 0.0f };
    float m_min          = 0.0f;
    float m_max          = 1.0f;
    std::string m_format = "%.3f";
    std::function<void(const Vec4&)> m_callback;
};
} // namespace aph

//
// Drawing Widgets
//
namespace aph
{
// DrawText widget
class DrawText : public Widget
{
public:
    explicit DrawText(UI* pUI);
    void setText(const std::string& text);
    const std::string& getText() const;
    void setColor(const Vec4& color);
    const Vec4& getColor() const;
    void draw() override;
    WidgetType getType() const override;

private:
    std::string m_text;
    Vec4 m_color = { 1.0f, 1.0f, 1.0f, 1.0f };
};

// DrawTooltip widget
class DrawTooltip : public Widget
{
public:
    explicit DrawTooltip(UI* pUI);
    void draw() override;
    WidgetType getType() const override;
};

// DrawLine widget
class DrawLine : public Widget
{
public:
    explicit DrawLine(UI* pUI);
    void setEndPoint(const Vec2& end);
    const Vec2& getEndPoint() const;
    void setColor(const Vec4& color);
    const Vec4& getColor() const;
    void setThickness(float thickness);
    float getThickness() const;
    void draw() override;
    WidgetType getType() const override;

private:
    Vec2 m_end        = { 0.0f, 0.0f };
    Vec4 m_color      = { 1.0f, 1.0f, 1.0f, 1.0f };
    float m_thickness = 1.0f;
};

// DrawCurve widget
class DrawCurve : public Widget
{
public:
    explicit DrawCurve(UI* pUI);
    void setControlPoints(const Vec2& cp1, const Vec2& cp2, const Vec2& end);
    void setColor(const Vec4& color);
    const Vec4& getColor() const;
    void setThickness(float thickness);
    float getThickness() const;
    void setSegments(int segments);
    int getSegments() const;
    void draw() override;
    WidgetType getType() const override;

private:
    Vec2 m_cp1        = { 0.0f, 0.0f };
    Vec2 m_cp2        = { 0.0f, 0.0f };
    Vec2 m_end        = { 0.0f, 0.0f };
    Vec4 m_color      = { 1.0f, 1.0f, 1.0f, 1.0f };
    float m_thickness = 1.0f;
    int m_segments    = 0;
};

// FilledRect widget
class FilledRect : public Widget
{
public:
    explicit FilledRect(UI* pUI);
    void setColor(const Vec4& color);
    const Vec4& getColor() const;
    void setRounding(float rounding);
    float getRounding() const;
    void setFlags(ImDrawFlags flags);
    ImDrawFlags getFlags() const;
    void draw() override;
    WidgetType getType() const override;

private:
    Vec4 m_color        = { 1.0f, 1.0f, 1.0f, 1.0f };
    float m_rounding    = 0.0f;
    ImDrawFlags m_flags = 0;
};
} // namespace aph

//
// Miscellaneous Widgets
//
namespace aph
{
// HorizontalSpace widget
class HorizontalSpace : public Widget
{
public:
    explicit HorizontalSpace(UI* pUI);
    void setWidth(float width);
    float getWidth() const;
    void draw() override;
    WidgetType getType() const override;

private:
    float m_width = 10.0f;
};

// VerticalSeparator widget
class VerticalSeparator : public Widget
{
public:
    explicit VerticalSeparator(UI* pUI);
    void setPadding(float padding);
    float getPadding() const;
    void draw() override;
    WidgetType getType() const override;

private:
    float m_padding = 3.0f;
};

// RadioButton widget
class RadioButton : public Widget
{
public:
    explicit RadioButton(UI* pUI);
    void setValue(bool value);
    bool getValue() const;
    void setCallback(std::function<void(bool)> callback);
    void draw() override;
    WidgetType getType() const override;

private:
    bool m_value = false;
    std::function<void(bool)> m_callback;
};

// SliderInt widget
class SliderInt : public Widget
{
public:
    explicit SliderInt(UI* pUI);
    void setValue(int value);
    int getValue() const;
    void setRange(int min, int max);
    void setFormat(const std::string& format);
    void setCallback(std::function<void(int)> callback);
    void draw() override;
    WidgetType getType() const override;

private:
    int m_value          = 0;
    int m_min            = 0;
    int m_max            = 100;
    std::string m_format = "%d";
    std::function<void(int)> m_callback;
};

// SliderUint widget
class SliderUint : public Widget
{
public:
    explicit SliderUint(UI* pUI);
    void setValue(uint32_t value);
    uint32_t getValue() const;
    void setRange(uint32_t min, uint32_t max);
    void setFormat(const std::string& format);
    void setCallback(std::function<void(uint32_t)> callback);
    void draw() override;
    WidgetType getType() const override;

private:
    uint32_t m_value     = 0;
    uint32_t m_min       = 0;
    uint32_t m_max       = 100;
    std::string m_format = "%u";
    std::function<void(uint32_t)> m_callback;
};

// OneLineCheckbox widget (checkbox + label on the same line)
class OneLineCheckbox : public Widget
{
public:
    explicit OneLineCheckbox(UI* pUI);
    void setValue(bool value);
    bool getValue() const;
    void setDescription(const std::string& description);
    const std::string& getDescription() const;
    void setCallback(std::function<void(bool)> callback);
    void draw() override;
    WidgetType getType() const override;

private:
    bool m_value = false;
    std::string m_description;
    std::function<void(bool)> m_callback;
};

// CursorLocation widget (shows cursor position)
class CursorLocation : public Widget
{
public:
    explicit CursorLocation(UI* pUI);
    void draw() override;
    WidgetType getType() const override;
};

// Column widget (for layout)
class Column : public Widget
{
public:
    explicit Column(UI* pUI);
    void setColumnCount(int count);
    int getColumnCount() const;
    void setBorders(bool showBorders);
    bool getBorders() const;
    void beginColumn(int index);
    void endColumns();
    void draw() override;
    WidgetType getType() const override;

private:
    int m_columnCount  = 2;
    bool m_showBorders = true;
};

// DynamicText widget (text that can be updated frequently)
class DynamicText : public Widget
{
public:
    explicit DynamicText(UI* pUI);
    void setText(const std::string& text);
    const std::string& getText() const;
    void setColor(const Vec4& color);
    const Vec4& getColor() const;
    void draw() override;
    WidgetType getType() const override;

private:
    std::string m_text;
    Vec4 m_color = { 1.0f, 1.0f, 1.0f, 1.0f };
};

// DebugTexture widget (displays a texture with debug information)
class DebugTexture : public Widget
{
public:
    explicit DebugTexture(UI* pUI);
    void setTextureID(ImTextureID id);
    ImTextureID getTextureID() const;
    void setShowInfo(bool show);
    bool getShowInfo() const;
    void draw() override;
    WidgetType getType() const override;

private:
    ImTextureID m_textureID = (ImTextureID)0;
    bool m_showInfo         = true;
};

class TreeNode : public Widget
{
public:
    explicit TreeNode(UI* pUI);
    void addWidget(Widget* widget);
    void setFlags(ImGuiTreeNodeFlags flags);
    ImGuiTreeNodeFlags getFlags() const;
    bool begin();
    void end();
    void draw() override;
    WidgetType getType() const override;

private:
    SmallVector<Widget*> m_widgets;
    ImGuiTreeNodeFlags m_flags = 0;
    bool m_isOpen              = false;
};

} // namespace aph
