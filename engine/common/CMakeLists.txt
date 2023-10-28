file(GLOB API_COMMON_SRC ${APH_ENGINE_COMMON_DIR}/*.cpp)
add_library(common STATIC ${API_COMMON_SRC})
aph_compiler_options(common)
target_include_directories(common PRIVATE
  ${APH_ENGINE_DIR}
  ${APH_EXTERNAL_DIR}/backward-cpp/include
)
