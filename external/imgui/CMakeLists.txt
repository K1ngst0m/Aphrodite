add_library(imgui STATIC
  ${APH_EXTERNAL_DIR}/imgui/imgui.cpp
  ${APH_EXTERNAL_DIR}/imgui/imgui_demo.cpp
  ${APH_EXTERNAL_DIR}/imgui/imgui_draw.cpp
  ${APH_EXTERNAL_DIR}/imgui/imgui_tables.cpp
  ${APH_EXTERNAL_DIR}/imgui/imgui_widgets.cpp
  ${APH_EXTERNAL_DIR}/imgui/imgui_impl_vulkan.cpp
)

target_include_directories(imgui PRIVATE ${APH_EXTERNAL_DIR}/glfw/include)
