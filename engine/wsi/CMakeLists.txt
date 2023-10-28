file(GLOB API_WSI_SRC ${APH_ENGINE_WSI_DIR}/*.cpp)
add_library(wsi STATIC ${API_WSI_SRC})
aph_compiler_options(wsi)
target_include_directories(wsi PRIVATE
  ${APH_ENGINE_DIR}
  ${APH_EXTERNAL_DIR}/volk
  ${APH_EXTERNAL_DIR}/imgui
  ${APH_EXTERNAL_DIR}/glm
)

target_link_libraries(wsi PRIVATE imgui glfw)
