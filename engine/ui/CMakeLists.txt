file(GLOB APH_UI_SRC ${APH_ENGINE_UI_DIR}/*.cpp)
add_library(aph_ui STATIC ${APH_UI_SRC})
aph_compiler_options(aph_ui)
target_include_directories(aph_ui PRIVATE
  ${APH_ENGINE_DIR}
  ${APH_EXTERNAL_DIR}/unordered_dense/include
)
