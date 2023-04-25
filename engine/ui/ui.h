#ifndef VULKAN_UIRENDERER_H_
#define VULKAN_UIRENDERER_H_

#include "renderer/renderer.h"

namespace aph::ui
{
void drawWithItemWidth(float itemWidth, float scale, std::function<void()>&& drawFunc);
void drawWindow(std::string_view title, glm::vec2 pos, glm::vec2 size, float scale, std::function<void()>&& drawFunc);
bool header(const char* caption);
bool checkBox(const char* caption, bool* value);
bool checkBox(const char* caption, int32_t* value);
bool radioButton(const char* caption, bool value);
bool sliderFloat(const char* caption, float* value, float min, float max);
bool sliderInt(const char* caption, int32_t* value, int32_t min, int32_t max);
bool comboBox(const char* caption, int32_t* itemindex, std::vector<std::string> items);
bool button(const char* caption);
bool colorPicker(const char* caption, float* color);
void text(const char* formatstr, ...);
}  // namespace aph::ui

#endif  // UIRENDERER_H_
