file(GLOB API_CLI_SRC ${APH_ENGINE_CLI_DIR}/*.cpp)
add_library(cli STATIC ${API_CLI_SRC})
aph_compiler_options(cli)
target_include_directories(cli PRIVATE
  ${APH_ENGINE_DIR}
  ${APH_EXTERNAL_DIR}/glm
)
