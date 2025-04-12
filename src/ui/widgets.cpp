#include "widgets.h"
#include "ui.h"

namespace aph
{

//
// Common Widgets
//

// Label widget
Label::Label(UI* pUI)
    : Widget(pUI)
{
}

void Label::draw()
{
    if (!m_enabled)
        return;

    ImGui::Text("%s", m_label.c_str());
}

WidgetType Label::getType() const
{
    return WidgetType::Label;
}

// Color Label widget
ColorLabel::ColorLabel(UI* pUI)
    : Widget(pUI)
{
}

void ColorLabel::setColor(const Vec4& color)
{
    m_color = color;
}

const Vec4& ColorLabel::getColor() const
{
    return m_color;
}

void ColorLabel::draw()
{
    if (!m_enabled)
        return;

    ImGui::TextColored(ImVec4(m_color.x, m_color.y, m_color.z, m_color.w), "%s", m_label.c_str());
}

WidgetType ColorLabel::getType() const
{
    return WidgetType::ColorLabel;
}

// Button widget
Button::Button(UI* pUI)
    : Widget(pUI)
{
}

void Button::setCallback(std::function<void()> callback)
{
    m_callback = std::move(callback);
}

void Button::draw()
{
    if (!m_enabled)
        return;

    if (m_size.x > 0 && m_size.y > 0)
    {
        if (ImGui::Button(m_label.c_str(), ImVec2(m_size.x, m_size.y)))
        {
            if (m_callback)
                m_callback();
        }
    }
    else
    {
        if (ImGui::Button(m_label.c_str()))
        {
            if (m_callback)
                m_callback();
        }
    }
}

WidgetType Button::getType() const
{
    return WidgetType::Button;
}

// Checkbox widget
Checkbox::Checkbox(UI* pUI)
    : Widget(pUI)
{
}

void Checkbox::setValue(bool value)
{
    m_value = value;
}

bool Checkbox::getValue() const
{
    return m_value;
}

void Checkbox::setCallback(std::function<void(bool)> callback)
{
    m_callback = std::move(callback);
}

void Checkbox::draw()
{
    if (!m_enabled)
        return;

    bool value = m_value;
    if (ImGui::Checkbox(m_label.c_str(), &value))
    {
        m_value = value;
        if (m_callback)
            m_callback(m_value);
    }
}

WidgetType Checkbox::getType() const
{
    return WidgetType::Checkbox;
}

// Slider Float widget
SliderFloat::SliderFloat(UI* pUI)
    : Widget(pUI)
{
}

void SliderFloat::setValue(float value)
{
    m_value = value;
}

float SliderFloat::getValue() const
{
    return m_value;
}

void SliderFloat::setRange(float min, float max)
{
    m_min = min;
    m_max = max;
}

void SliderFloat::setFormat(const std::string& format)
{
    m_format = format;
}

void SliderFloat::setCallback(std::function<void(float)> callback)
{
    m_callback = std::move(callback);
}

void SliderFloat::draw()
{
    if (!m_enabled)
        return;

    float value = m_value;
    if (ImGui::SliderFloat(m_label.c_str(), &value, m_min, m_max, m_format.c_str()))
    {
        m_value = value;
        if (m_callback)
        {
            m_callback(m_value);
        }
    }
}

WidgetType SliderFloat::getType() const
{
    return WidgetType::SliderFloat;
}

// Slider Float2 widget
SliderFloat2::SliderFloat2(UI* pUI)
    : Widget(pUI)
{
}

void SliderFloat2::setValue(const Vec2& value)
{
    m_value = value;
}

const Vec2& SliderFloat2::getValue() const
{
    return m_value;
}

void SliderFloat2::setRange(float min, float max)
{
    m_min = min;
    m_max = max;
}

void SliderFloat2::setFormat(const std::string& format)
{
    m_format = format;
}

void SliderFloat2::setCallback(std::function<void(const Vec2&)> callback)
{
    m_callback = std::move(callback);
}

void SliderFloat2::draw()
{
    if (!m_enabled)
        return;

    float values[2] = {m_value.x, m_value.y};
    if (ImGui::SliderFloat2(m_label.c_str(), values, m_min, m_max, m_format.c_str()))
    {
        m_value.x = values[0];
        m_value.y = values[1];
        if (m_callback)
            m_callback(m_value);
    }
}

WidgetType SliderFloat2::getType() const
{
    return WidgetType::SliderFloat2;
}

// Dropdown widget
Dropdown::Dropdown(UI* pUI)
    : Widget(pUI)
{
}

void Dropdown::setOptions(const SmallVector<std::string>& options)
{
    m_options = options;
}

void Dropdown::setSelectedIndex(int index)
{
    m_selectedIndex = index;
}

int Dropdown::getSelectedIndex() const
{
    return m_selectedIndex;
}

const std::string& Dropdown::getSelectedOption() const
{
    APH_ASSERT(m_selectedIndex >= 0 && m_selectedIndex < m_options.size());
    return m_options[m_selectedIndex];
}

void Dropdown::setCallback(std::function<void(int, const std::string&)> callback)
{
    m_callback = std::move(callback);
}

void Dropdown::draw()
{
    if (!m_enabled || m_options.empty())
        return;

    if (ImGui::BeginCombo(m_label.c_str(), m_options[m_selectedIndex].c_str()))
    {
        for (int i = 0; i < m_options.size(); i++)
        {
            bool isSelected = (i == m_selectedIndex);
            if (ImGui::Selectable(m_options[i].c_str(), isSelected))
            {
                m_selectedIndex = i;
                if (m_callback)
                    m_callback(m_selectedIndex, m_options[m_selectedIndex]);
            }

            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

WidgetType Dropdown::getType() const
{
    return WidgetType::Dropdown;
}

// Collapsing Header widget
CollapsingHeader::CollapsingHeader(UI* pUI)
    : Widget(pUI)
{
}

void CollapsingHeader::addWidget(Widget* widget)
{
    APH_ASSERT(widget);
    m_widgets.push_back(widget);
}

void CollapsingHeader::draw()
{
    if (!m_enabled)
    {
        return;
    }

    if (ImGui::CollapsingHeader(m_label.c_str(), m_flags))
    {
        for (auto widget : m_widgets)
        {
            if (widget && widget->isEnabled())
            {
                widget->draw();
            }
        }
    }
}

void CollapsingHeader::setFlags(ImGuiTreeNodeFlags flags)
{
    m_flags = flags;
}

size_t CollapsingHeader::getWidgetCount() const
{
    return m_widgets.size();
}

void CollapsingHeader::removeWidget(size_t index)
{
    if (index < m_widgets.size())
    {
        m_widgets.erase(m_widgets.begin() + index);
    }
}

WidgetType CollapsingHeader::getType() const
{
    return WidgetType::CollapsingHeader;
}

// Separator widget
Separator::Separator(UI* pUI)
    : Widget(pUI)
{
}

void Separator::draw()
{
    if (!m_enabled)
        return;

    ImGui::Separator();
}

WidgetType Separator::getType() const
{
    return WidgetType::Separator;
}

// Progress Bar widget
ProgressBar::ProgressBar(UI* pUI)
    : Widget(pUI)
{
}

void ProgressBar::setValue(float value)
{
    m_value = value;
}

float ProgressBar::getValue() const
{
    return m_value;
}

void ProgressBar::draw()
{
    if (!m_enabled)
        return;

    if (m_size.x > 0)
    {
        ImGui::ProgressBar(m_value, ImVec2(m_size.x, m_size.y), m_label.empty() ? nullptr : m_label.c_str());
    }
    else
    {
        ImGui::ProgressBar(m_value, ImVec2(-1, 0), m_label.empty() ? nullptr : m_label.c_str());
    }
}

WidgetType ProgressBar::getType() const
{
    return WidgetType::ProgressBar;
}

// Custom widget for implementing specialized behavior
CustomWidget::CustomWidget(UI* pUI)
    : Widget(pUI)
{
}

void CustomWidget::setDrawCallback(std::function<void()> callback)
{
    m_drawCallback = std::move(callback);
}

void CustomWidget::draw()
{
    if (!m_enabled)
        return;

    if (m_drawCallback)
        m_drawCallback();
}

WidgetType CustomWidget::getType() const
{
    return WidgetType::Custom;
}

//
// Container Widgets
//

// Container widget for managing groups of widgets
WidgetContainer::WidgetContainer(UI* pUI)
    : m_pUI(pUI)
{
}

ContainerType WidgetContainer::getType() const
{
    return ContainerType::Generic;
}

void WidgetContainer::drawAll()
{
    for (auto widget : m_widgets)
    {
        if (widget && widget->isEnabled())
        {
            // Add breadcrumb for each widget being drawn
            if (m_pUI)
            {
                // Use the ToString function instead of a switch statement
                std::string widgetType = ToString(widget->getType());
                // Set leaf node status for prettier output
                bool isLast = (widget == m_widgets.back());

                // Add breadcrumb with proper indentation (one level deeper than container)
                m_pUI->addBreadcrumb("DrawWidget", widgetType + ": " + widget->getLabel(), BreadcrumbLevel::Widget,
                                     isLast);

                widget->draw();
            }
            else
            {
                widget->draw();
            }
        }
    }
}

void WidgetContainer::clear()
{
    // We don't free the widgets here because the UI owns them through the widget pool
    m_widgets.clear();
}

void WidgetContainer::setAllEnabled(bool enabled)
{
    for (auto widget : m_widgets)
    {
        if (widget)
            widget->setEnabled(enabled);
    }
}

size_t WidgetContainer::size() const
{
    return m_widgets.size();
}

// A window that contains widgets
WidgetWindow::WidgetWindow(UI* pUI)
    : WidgetContainer(pUI)
{
}

ContainerType WidgetWindow::getType() const
{
    return ContainerType::Window;
}

void WidgetWindow::setTitle(const std::string& title)
{
    m_title = title;
}

const std::string& WidgetWindow::getTitle() const
{
    return m_title;
}

void WidgetWindow::setSize(const Vec2& size)
{
    m_size = size;
}

const Vec2& WidgetWindow::getSize() const
{
    return m_size;
}

void WidgetWindow::setPosition(const Vec2& position)
{
    m_position = position;
}

const Vec2& WidgetWindow::getPosition() const
{
    return m_position;
}

void WidgetWindow::setFlags(ImGuiWindowFlags flags)
{
    m_flags = flags;
}

ImGuiWindowFlags WidgetWindow::getFlags() const
{
    return m_flags;
}

bool WidgetWindow::begin()
{
    if (m_size.x > 0 && m_size.y > 0)
    {
        ImGui::SetNextWindowSize(ImVec2(m_size.x, m_size.y), ImGuiCond_FirstUseEver);
    }

    if (m_position.x > 0 || m_position.y > 0)
    {
        ImGui::SetNextWindowPos(ImVec2(m_position.x, m_position.y), ImGuiCond_FirstUseEver);
    }

    return ImGui::Begin(m_title.c_str(), &m_open, m_flags);
}

void WidgetWindow::end()
{
    ImGui::End();
}

bool WidgetWindow::isOpen() const
{
    return m_open;
}

void WidgetWindow::setOpen(bool open)
{
    m_open = open;
}

void WidgetWindow::draw()
{
    if (!m_open)
        return;

    if (m_pUI)
    {
        m_pUI->addBreadcrumb("BeginWindow", m_title, BreadcrumbLevel::Widget);
    }

    if (begin())
    {
        drawAll();
    }
    end();

    if (m_pUI)
    {
        m_pUI->addBreadcrumb("EndWindow", m_title, BreadcrumbLevel::Widget, true);
    }
}

//
// Advanced Widgets
//

// Color Picker widget
ColorPicker::ColorPicker(UI* pUI)
    : Widget(pUI)
{
}

void ColorPicker::setColor(const Vec4& color)
{
    m_color = color;
}

const Vec4& ColorPicker::getColor() const
{
    return m_color;
}

void ColorPicker::setFlags(ImGuiColorEditFlags flags)
{
    m_flags = flags;
}

void ColorPicker::setCallback(std::function<void(const Vec4&)> callback)
{
    m_callback = std::move(callback);
}

void ColorPicker::draw()
{
    if (!m_enabled)
        return;

    float color[4] = {m_color.x, m_color.y, m_color.z, m_color.w};
    if (ImGui::ColorEdit4(m_label.c_str(), color, m_flags))
    {
        m_color = {color[0], color[1], color[2], color[3]};
        if (m_callback)
            m_callback(m_color);
    }
}

WidgetType ColorPicker::getType() const
{
    return WidgetType::ColorPicker;
}

// Color3 Picker widget (RGB only)
Color3Picker::Color3Picker(UI* pUI)
    : Widget(pUI)
{
}

void Color3Picker::setColor(const Vec3& color)
{
    m_color = color;
}

const Vec3& Color3Picker::getColor() const
{
    return m_color;
}

void Color3Picker::setFlags(ImGuiColorEditFlags flags)
{
    m_flags = flags;
}

void Color3Picker::setCallback(std::function<void(const Vec3&)> callback)
{
    m_callback = std::move(callback);
}

void Color3Picker::draw()
{
    if (!m_enabled)
        return;

    float color[3] = {m_color.x, m_color.y, m_color.z};
    if (ImGui::ColorEdit3(m_label.c_str(), color, m_flags))
    {
        m_color = {color[0], color[1], color[2]};
        if (m_callback)
            m_callback(m_color);
    }
}

WidgetType Color3Picker::getType() const
{
    return WidgetType::Color3Picker;
}

// Plot Lines widget
PlotLines::PlotLines(UI* pUI)
    : Widget(pUI)
{
}

void PlotLines::setValues(const SmallVector<float>& values)
{
    m_values = values;
}

const SmallVector<float>& PlotLines::getValues() const
{
    return m_values;
}

void PlotLines::setScaleMin(float min)
{
    m_scaleMin = min;
}

void PlotLines::setScaleMax(float max)
{
    m_scaleMax = max;
}

void PlotLines::setOverlayText(const std::string& text)
{
    m_overlayText = text;
}

void PlotLines::draw()
{
    if (!m_enabled || m_values.empty())
        return;

    if (m_size.x > 0 && m_size.y > 0)
    {
        ImGui::PlotLines(m_label.c_str(), m_values.data(), static_cast<int>(m_values.size()), 0,
                         m_overlayText.empty() ? nullptr : m_overlayText.c_str(), m_scaleMin, m_scaleMax,
                         ImVec2(m_size.x, m_size.y));
    }
    else
    {
        ImGui::PlotLines(m_label.c_str(), m_values.data(), static_cast<int>(m_values.size()), 0,
                         m_overlayText.empty() ? nullptr : m_overlayText.c_str(), m_scaleMin, m_scaleMax);
    }
}

WidgetType PlotLines::getType() const
{
    return WidgetType::PlotLines;
}

// Histogram widget
Histogram::Histogram(UI* pUI)
    : Widget(pUI)
{
}

void Histogram::setValues(const SmallVector<float>& values)
{
    m_values = values;
}

const SmallVector<float>& Histogram::getValues() const
{
    return m_values;
}

void Histogram::setScaleMin(float min)
{
    m_scaleMin = min;
}

void Histogram::setScaleMax(float max)
{
    m_scaleMax = max;
}

void Histogram::setOverlayText(const std::string& text)
{
    m_overlayText = text;
}

void Histogram::draw()
{
    if (!m_enabled || m_values.empty())
        return;

    if (m_size.x > 0 && m_size.y > 0)
    {
        ImGui::PlotHistogram(m_label.c_str(), m_values.data(), static_cast<int>(m_values.size()), 0,
                             m_overlayText.empty() ? nullptr : m_overlayText.c_str(), m_scaleMin, m_scaleMax,
                             ImVec2(m_size.x, m_size.y));
    }
    else
    {
        ImGui::PlotHistogram(m_label.c_str(), m_values.data(), static_cast<int>(m_values.size()), 0,
                             m_overlayText.empty() ? nullptr : m_overlayText.c_str(), m_scaleMin, m_scaleMax);
    }
}

WidgetType Histogram::getType() const
{
    return WidgetType::Histogram;
}

// TextBox widget
TextBox::TextBox(UI* pUI)
    : Widget(pUI)
{
}

void TextBox::setText(const std::string& text)
{
    m_text = text;
    m_buffer.resize(m_text.size() + m_bufferExtraSize);
    strcpy(m_buffer.data(), m_text.c_str());
}

const std::string& TextBox::getText() const
{
    return m_text;
}

void TextBox::setBufferSize(size_t size)
{
    m_bufferExtraSize = size;
    m_buffer.resize(m_text.size() + m_bufferExtraSize);
    strcpy(m_buffer.data(), m_text.c_str());
}

void TextBox::setFlags(ImGuiInputTextFlags flags)
{
    m_flags = flags;
}

void TextBox::setCallback(std::function<void(const std::string&)> callback)
{
    m_callback = std::move(callback);
}

void TextBox::setMultiline(bool multiline)
{
    m_multiline = multiline;
}

void TextBox::draw()
{
    if (!m_enabled)
        return;

    bool changed = false;

    if (m_multiline)
    {
        if (m_size.x > 0 && m_size.y > 0)
        {
            changed = ImGui::InputTextMultiline(m_label.c_str(), m_buffer.data(), m_buffer.size(),
                                                ImVec2(m_size.x, m_size.y), m_flags);
        }
        else
        {
            changed =
                ImGui::InputTextMultiline(m_label.c_str(), m_buffer.data(), m_buffer.size(), ImVec2(0, 0), m_flags);
        }
    }
    else
    {
        changed = ImGui::InputText(m_label.c_str(), m_buffer.data(), m_buffer.size(), m_flags);
    }

    if (changed)
    {
        m_text = m_buffer.data();
        if (m_callback)
            m_callback(m_text);
    }
}

WidgetType TextBox::getType() const
{
    return WidgetType::TextBox;
}

// Slider Float3 widget
SliderFloat3::SliderFloat3(UI* pUI)
    : Widget(pUI)
{
}

void SliderFloat3::setValue(const Vec3& value)
{
    m_value = value;
}

const Vec3& SliderFloat3::getValue() const
{
    return m_value;
}

void SliderFloat3::setRange(float min, float max)
{
    m_min = min;
    m_max = max;
}

void SliderFloat3::setFormat(const std::string& format)
{
    m_format = format;
}

void SliderFloat3::setCallback(std::function<void(const Vec3&)> callback)
{
    m_callback = std::move(callback);
}

void SliderFloat3::draw()
{
    if (!m_enabled)
        return;

    float values[3] = {m_value.x, m_value.y, m_value.z};
    if (ImGui::SliderFloat3(m_label.c_str(), values, m_min, m_max, m_format.c_str()))
    {
        m_value.x = values[0];
        m_value.y = values[1];
        m_value.z = values[2];
        if (m_callback)
            m_callback(m_value);
    }
}

WidgetType SliderFloat3::getType() const
{
    return WidgetType::SliderFloat3;
}

// Slider Float4 widget
SliderFloat4::SliderFloat4(UI* pUI)
    : Widget(pUI)
{
}

void SliderFloat4::setValue(const Vec4& value)
{
    m_value = value;
}

const Vec4& SliderFloat4::getValue() const
{
    return m_value;
}

void SliderFloat4::setRange(float min, float max)
{
    m_min = min;
    m_max = max;
}

void SliderFloat4::setFormat(const std::string& format)
{
    m_format = format;
}

void SliderFloat4::setCallback(std::function<void(const Vec4&)> callback)
{
    m_callback = std::move(callback);
}

void SliderFloat4::draw()
{
    if (!m_enabled)
        return;

    float values[4] = {m_value.x, m_value.y, m_value.z, m_value.w};
    if (ImGui::SliderFloat4(m_label.c_str(), values, m_min, m_max, m_format.c_str()))
    {
        m_value.x = values[0];
        m_value.y = values[1];
        m_value.z = values[2];
        m_value.w = values[3];
        if (m_callback)
            m_callback(m_value);
    }
}

WidgetType SliderFloat4::getType() const
{
    return WidgetType::SliderFloat4;
}

//
// Drawing Widgets
//

// DrawText widget
DrawText::DrawText(UI* pUI)
    : Widget(pUI)
{
}

void DrawText::setText(const std::string& text)
{
    m_text = text;
}

const std::string& DrawText::getText() const
{
    return m_text;
}

void DrawText::setColor(const Vec4& color)
{
    m_color = color;
}

const Vec4& DrawText::getColor() const
{
    return m_color;
}

void DrawText::draw()
{
    if (!m_enabled)
        return;

    ImGui::GetWindowDrawList()->AddText(
        ImVec2(m_position.x, m_position.y),
        ImGui::ColorConvertFloat4ToU32(ImVec4(m_color.x, m_color.y, m_color.z, m_color.w)), m_text.c_str());
}

WidgetType DrawText::getType() const
{
    return WidgetType::DrawText;
}

// DrawTooltip widget
DrawTooltip::DrawTooltip(UI* pUI)
    : Widget(pUI)
{
}

void DrawTooltip::draw()
{
    if (!m_enabled || m_label.empty())
        return;

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("%s", m_label.c_str());
        ImGui::EndTooltip();
    }
}

WidgetType DrawTooltip::getType() const
{
    return WidgetType::DrawTooltip;
}

// DrawLine widget
DrawLine::DrawLine(UI* pUI)
    : Widget(pUI)
{
}

void DrawLine::setEndPoint(const Vec2& end)
{
    m_end = end;
}

const Vec2& DrawLine::getEndPoint() const
{
    return m_end;
}

void DrawLine::setColor(const Vec4& color)
{
    m_color = color;
}

const Vec4& DrawLine::getColor() const
{
    return m_color;
}

void DrawLine::setThickness(float thickness)
{
    m_thickness = thickness;
}

float DrawLine::getThickness() const
{
    return m_thickness;
}

void DrawLine::draw()
{
    if (!m_enabled)
        return;

    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(m_position.x, m_position.y), ImVec2(m_end.x, m_end.y),
        ImGui::ColorConvertFloat4ToU32(ImVec4(m_color.x, m_color.y, m_color.z, m_color.w)), m_thickness);
}

WidgetType DrawLine::getType() const
{
    return WidgetType::DrawLine;
}

// DrawCurve widget
DrawCurve::DrawCurve(UI* pUI)
    : Widget(pUI)
{
}

void DrawCurve::setControlPoints(const Vec2& cp1, const Vec2& cp2, const Vec2& end)
{
    m_cp1 = cp1;
    m_cp2 = cp2;
    m_end = end;
}

void DrawCurve::setColor(const Vec4& color)
{
    m_color = color;
}

const Vec4& DrawCurve::getColor() const
{
    return m_color;
}

void DrawCurve::setThickness(float thickness)
{
    m_thickness = thickness;
}

float DrawCurve::getThickness() const
{
    return m_thickness;
}

void DrawCurve::setSegments(int segments)
{
    m_segments = segments;
}

int DrawCurve::getSegments() const
{
    return m_segments;
}

void DrawCurve::draw()
{
    if (!m_enabled)
        return;

    ImGui::GetWindowDrawList()->AddBezierCubic(
        ImVec2(m_position.x, m_position.y), ImVec2(m_cp1.x, m_cp1.y), ImVec2(m_cp2.x, m_cp2.y),
        ImVec2(m_end.x, m_end.y), ImGui::ColorConvertFloat4ToU32(ImVec4(m_color.x, m_color.y, m_color.z, m_color.w)),
        m_thickness, m_segments);
}

WidgetType DrawCurve::getType() const
{
    return WidgetType::DrawCurve;
}

// FilledRect widget
FilledRect::FilledRect(UI* pUI)
    : Widget(pUI)
{
}

void FilledRect::setColor(const Vec4& color)
{
    m_color = color;
}

const Vec4& FilledRect::getColor() const
{
    return m_color;
}

void FilledRect::setRounding(float rounding)
{
    m_rounding = rounding;
}

float FilledRect::getRounding() const
{
    return m_rounding;
}

void FilledRect::setFlags(ImDrawFlags flags)
{
    m_flags = flags;
}

ImDrawFlags FilledRect::getFlags() const
{
    return m_flags;
}

void FilledRect::draw()
{
    if (!m_enabled)
        return;

    ImGui::GetWindowDrawList()->AddRectFilled(
        ImVec2(m_position.x, m_position.y), ImVec2(m_position.x + m_size.x, m_position.y + m_size.y),
        ImGui::ColorConvertFloat4ToU32(ImVec4(m_color.x, m_color.y, m_color.z, m_color.w)), m_rounding, m_flags);
}

WidgetType FilledRect::getType() const
{
    return WidgetType::FilledRect;
}

//
// Miscellaneous Widgets
//

// HorizontalSpace widget
HorizontalSpace::HorizontalSpace(UI* pUI)
    : Widget(pUI)
{
}

void HorizontalSpace::setWidth(float width)
{
    m_width = width;
}

float HorizontalSpace::getWidth() const
{
    return m_width;
}

void HorizontalSpace::draw()
{
    if (!m_enabled)
        return;

    ImGui::SameLine();
    ImGui::Dummy(ImVec2(m_width, 0.0f));
}

WidgetType HorizontalSpace::getType() const
{
    return WidgetType::HorizontalSpace;
}

// VerticalSeparator widget
VerticalSeparator::VerticalSeparator(UI* pUI)
    : Widget(pUI)
{
}

void VerticalSeparator::setPadding(float padding)
{
    m_padding = padding;
}

float VerticalSeparator::getPadding() const
{
    return m_padding;
}

void VerticalSeparator::draw()
{
    if (!m_enabled)
        return;

    ImGui::SameLine(0, m_padding);
    // Use a colored rectangle instead of SeparatorEx
    ImVec2 screen_pos = ImGui::GetCursorScreenPos();
    ImVec2 screen_size(1.0f, ImGui::GetContentRegionAvail().y);
    ImGui::GetWindowDrawList()->AddRectFilled(screen_pos,
                                              ImVec2(screen_pos.x + screen_size.x, screen_pos.y + screen_size.y),
                                              ImGui::GetColorU32(ImGuiCol_Separator));
    ImGui::Dummy(ImVec2(1.0f, 0)); // Create space for the separator
    ImGui::SameLine(0, m_padding);
}

WidgetType VerticalSeparator::getType() const
{
    return WidgetType::VerticalSeparator;
}

// RadioButton widget
RadioButton::RadioButton(UI* pUI)
    : Widget(pUI)
{
}

void RadioButton::setValue(bool value)
{
    m_value = value;
}

bool RadioButton::getValue() const
{
    return m_value;
}

void RadioButton::setCallback(std::function<void(bool)> callback)
{
    m_callback = std::move(callback);
}

void RadioButton::draw()
{
    if (!m_enabled)
        return;

    bool value = m_value;
    if (ImGui::RadioButton(m_label.c_str(), value))
    {
        m_value = value;
        if (m_callback)
            m_callback(m_value);
    }
}

WidgetType RadioButton::getType() const
{
    return WidgetType::RadioButton;
}

// SliderInt widget
SliderInt::SliderInt(UI* pUI)
    : Widget(pUI)
{
}

void SliderInt::setValue(int value)
{
    m_value = value;
}

int SliderInt::getValue() const
{
    return m_value;
}

void SliderInt::setRange(int min, int max)
{
    m_min = min;
    m_max = max;
}

void SliderInt::setFormat(const std::string& format)
{
    m_format = format;
}

void SliderInt::setCallback(std::function<void(int)> callback)
{
    m_callback = std::move(callback);
}

void SliderInt::draw()
{
    if (!m_enabled)
        return;

    int value = m_value;
    if (ImGui::SliderInt(m_label.c_str(), &value, m_min, m_max, m_format.c_str()))
    {
        m_value = value;
        if (m_callback)
            m_callback(m_value);
    }
}

WidgetType SliderInt::getType() const
{
    return WidgetType::SliderInt;
}

// SliderUint widget
SliderUint::SliderUint(UI* pUI)
    : Widget(pUI)
{
}

void SliderUint::setValue(uint32_t value)
{
    m_value = value;
}

uint32_t SliderUint::getValue() const
{
    return m_value;
}

void SliderUint::setRange(uint32_t min, uint32_t max)
{
    m_min = min;
    m_max = max;
}

void SliderUint::setFormat(const std::string& format)
{
    m_format = format;
}

void SliderUint::setCallback(std::function<void(uint32_t)> callback)
{
    m_callback = std::move(callback);
}

void SliderUint::draw()
{
    if (!m_enabled)
        return;

    int value = static_cast<int>(m_value);
    if (ImGui::SliderInt(m_label.c_str(), &value, static_cast<int>(m_min), static_cast<int>(m_max), m_format.c_str()))
    {
        m_value = static_cast<uint32_t>(value);
        if (m_callback)
            m_callback(m_value);
    }
}

WidgetType SliderUint::getType() const
{
    return WidgetType::SliderUint;
}

// OneLineCheckbox widget (checkbox + label on the same line)
OneLineCheckbox::OneLineCheckbox(UI* pUI)
    : Widget(pUI)
{
}

void OneLineCheckbox::setValue(bool value)
{
    m_value = value;
}

bool OneLineCheckbox::getValue() const
{
    return m_value;
}

void OneLineCheckbox::setDescription(const std::string& description)
{
    m_description = description;
}

const std::string& OneLineCheckbox::getDescription() const
{
    return m_description;
}

void OneLineCheckbox::setCallback(std::function<void(bool)> callback)
{
    m_callback = std::move(callback);
}

void OneLineCheckbox::draw()
{
    if (!m_enabled)
        return;

    bool value = m_value;
    ImGui::Text("%s", m_label.c_str());
    ImGui::SameLine();
    if (ImGui::Checkbox(m_description.empty() ? "##" : m_description.c_str(), &value))
    {
        m_value = value;
        if (m_callback)
            m_callback(m_value);
    }
}

WidgetType OneLineCheckbox::getType() const
{
    return WidgetType::OneLineCheckbox;
}

// CursorLocation widget (shows cursor position)
CursorLocation::CursorLocation(UI* pUI)
    : Widget(pUI)
{
}

void CursorLocation::draw()
{
    if (!m_enabled)
        return;

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::Text("%s X: %.1f, Y: %.1f", m_label.c_str(), pos.x, pos.y);
}

WidgetType CursorLocation::getType() const
{
    return WidgetType::CursorLocation;
}

// Column widget (for layout)
Column::Column(UI* pUI)
    : Widget(pUI)
{
}

void Column::setColumnCount(int count)
{
    m_columnCount = count;
}

int Column::getColumnCount() const
{
    return m_columnCount;
}

void Column::setBorders(bool showBorders)
{
    m_showBorders = showBorders;
}

bool Column::getBorders() const
{
    return m_showBorders;
}

void Column::beginColumn(int index)
{
    if (index < 0 || index >= m_columnCount)
        return;

    if (index == 0)
    {
        ImGui::Columns(m_columnCount, m_label.empty() ? nullptr : m_label.c_str(), m_showBorders);
    }
    else
    {
        ImGui::NextColumn();
    }
}

void Column::endColumns()
{
    ImGui::Columns(1);
}

void Column::draw()
{
    // This widget is special and doesn't automatically draw anything
    // It's controlled manually with beginColumn and endColumns
    return;
}

WidgetType Column::getType() const
{
    return WidgetType::Column;
}

// DynamicText widget (text that can be updated frequently)
DynamicText::DynamicText(UI* pUI)
    : Widget(pUI)
{
}

void DynamicText::setText(const std::string& text)
{
    m_text = text;
}

const std::string& DynamicText::getText() const
{
    return m_text;
}

void DynamicText::setColor(const Vec4& color)
{
    m_color = color;
}

const Vec4& DynamicText::getColor() const
{
    return m_color;
}

void DynamicText::draw()
{
    if (!m_enabled)
        return;

    if (m_color.x != 1.0f || m_color.y != 1.0f || m_color.z != 1.0f || m_color.w != 1.0f)
    {
        ImGui::TextColored(ImVec4(m_color.x, m_color.y, m_color.z, m_color.w), "%s%s",
                           m_label.empty() ? "" : (m_label + ": ").c_str(), m_text.c_str());
    }
    else
    {
        ImGui::Text("%s%s", m_label.empty() ? "" : (m_label + ": ").c_str(), m_text.c_str());
    }
}

WidgetType DynamicText::getType() const
{
    return WidgetType::DynamicText;
}

// DebugTexture widget (displays a texture with debug information)
DebugTexture::DebugTexture(UI* pUI)
    : Widget(pUI)
{
}

void DebugTexture::setTextureID(ImTextureID id)
{
    m_textureID = id;
}

ImTextureID DebugTexture::getTextureID() const
{
    return m_textureID;
}

void DebugTexture::setShowInfo(bool show)
{
    m_showInfo = show;
}

bool DebugTexture::getShowInfo() const
{
    return m_showInfo;
}

void DebugTexture::draw()
{
    if (!m_enabled || !m_textureID)
        return;

    if (m_size.x <= 0)
        m_size.x = 256;
    if (m_size.y <= 0)
        m_size.y = 256;

    ImGui::Text("%s", m_label.c_str());
    ImGui::Image(m_textureID, ImVec2(m_size.x, m_size.y));

    if (m_showInfo && ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        float region_sz = 32.0f;
        ImVec2 uv0      = ImVec2(0, 0);
        ImVec2 uv1      = ImVec2(1, 1);
        ImGui::Image(m_textureID, ImVec2(region_sz * 4.0f, region_sz * 4.0f), uv0, uv1, ImVec4(1, 1, 1, 1),
                     ImVec4(1, 1, 1, 0.5f));
        ImGui::Text("ID: %llu", (unsigned long long)m_textureID);
        ImGui::Text("Size: %.0fx%.0f", m_size.x, m_size.y);
        ImGui::EndTooltip();
    }
}

WidgetType DebugTexture::getType() const
{
    return WidgetType::DebugTexture;
}

// Tree Node widget
TreeNode::TreeNode(UI* pUI)
    : Widget(pUI)
{
}

void TreeNode::addWidget(Widget* widget)
{
    APH_ASSERT(widget);
    m_widgets.push_back(widget);
}

void TreeNode::setFlags(ImGuiTreeNodeFlags flags)
{
    m_flags = flags;
}

ImGuiTreeNodeFlags TreeNode::getFlags() const
{
    return m_flags;
}

bool TreeNode::begin()
{
    m_isOpen = ImGui::TreeNodeEx(m_label.c_str(), m_flags);
    return m_isOpen;
}

void TreeNode::end()
{
    if (m_isOpen)
    {
        ImGui::TreePop();
    }
}

void TreeNode::draw()
{
    if (!m_enabled)
        return;

    if (begin())
    {
        for (auto widget : m_widgets)
        {
            if (widget && widget->isEnabled())
            {
                widget->draw();
            }
        }
        end();
    }
}

WidgetType TreeNode::getType() const
{
    return WidgetType::TreeNode;
}

} // namespace aph
