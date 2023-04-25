#include "ui.h"

#include "api/gpuResource.h"
#include "common/assetManager.h"

#include "api/vulkan/device.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

namespace aph::ui
{
void text(const char* formatstr, ...)
{
    va_list args;
    va_start(args, formatstr);
    ImGui::TextV(formatstr, args);
    va_end(args);
}

bool colorPicker(const char* caption, float* color)
{
    bool res = ImGui::ColorEdit4(caption, color, ImGuiColorEditFlags_NoInputs);
    return res;
}
bool button(const char* caption)
{
    bool res = ImGui::Button(caption);
    return res;
}
bool comboBox(const char* caption, int32_t* itemindex, std::vector<std::string> items)
{
    if(items.empty()) { return false; }
    std::vector<const char*> charitems;
    charitems.reserve(items.size());
    for(auto& item : items)
    {
        charitems.push_back(item.c_str());
    }
    uint32_t itemCount = static_cast<uint32_t>(charitems.size());
    bool     res       = ImGui::Combo(caption, itemindex, &charitems[0], itemCount, itemCount);
    return res;
}
bool sliderInt(const char* caption, int32_t* value, int32_t min, int32_t max)
{
    bool res = ImGui::SliderInt(caption, value, min, max);
    return res;
}
bool sliderFloat(const char* caption, float* value, float min, float max)
{
    bool res = ImGui::SliderFloat(caption, value, min, max);
    return res;
}
bool checkBox(const char* caption, int32_t* value)
{
    bool val = (*value == 1);
    bool res = ImGui::Checkbox(caption, &val);
    *value   = val;
    return res;
}
bool checkBox(const char* caption, bool* value)
{
    bool res = ImGui::Checkbox(caption, value);
    return res;
}
bool radioButton(const char* caption, bool value)
{
    bool res = ImGui::RadioButton(caption, value);
    return res;
}
bool header(const char* caption) { return ImGui::CollapsingHeader(caption, ImGuiTreeNodeFlags_DefaultOpen); }

void drawWithItemWidth(float itemWidth, float scale, std::function<void()>&& drawFunc)
{
    ImGui::PushItemWidth(itemWidth * scale);
    drawFunc();
    ImGui::PopItemWidth();
}

void drawWindow(std::string_view title, glm::vec2 pos, glm::vec2 size, float scale, std::function<void()>&& drawFunc)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::SetNextWindowPos(ImVec2(pos.x * scale, pos.y * scale));
    ImGui::SetNextWindowSize(ImVec2(size.x, size.y), ImGuiCond_FirstUseEver);
    ImGui::Begin(title.data(), nullptr,
                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    drawFunc();
    ImGui::PopStyleVar();
    ImGui::End();
}
}  // namespace aph::ui
