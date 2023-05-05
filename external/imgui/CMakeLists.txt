add_library(imgui
  imgui.cpp
  imgui_draw.cpp
  imgui_tables.cpp
  imgui_widgets.cpp
)
target_include_directories(imgui PRIVATE include/imgui)
