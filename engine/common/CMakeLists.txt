file(GLOB APH_COMMON_SRC ${APH_ENGINE_COMMON_DIR}/*.cpp)
add_library(common STATIC ${APH_COMMON_SRC})
aph_compiler_options(common)
target_include_directories(common PUBLIC
  ${APH_ENGINE_DIR}
  ${APH_EXTERNAL_DIR}/unordered_dense/include
  ${APH_EXTERNAL_DIR}/tracy/public

  PRIVATE
  ${APH_EXTERNAL_DIR}/backward-cpp/include
)
target_link_libraries(common PUBLIC
  unordered_dense::unordered_dense
  Tracy::TracyClient
)
