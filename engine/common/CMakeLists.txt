file(GLOB APH_COMMON_SRC ${APH_ENGINE_COMMON_DIR}/*.cpp)
add_library(common STATIC ${APH_COMMON_SRC})
aph_compiler_options(common)
target_include_directories(common PUBLIC
  ${APH_ENGINE_DIR}
)

target_compile_definitions(common
  PRIVATE
  $<$<BOOL:${APH_ENABLE_TRACING}>:APH_ENABLE_TRACY>
)

target_link_libraries(common
  PUBLIC
  unordered_dense::unordered_dense
  tomlplusplus::tomlplusplus
  $<$<BOOL:${APH_ENABLE_TRACING}>:Tracy::TracyClient>
  PRIVATE
  Backward::Interface
)
