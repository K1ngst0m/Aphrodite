file(GLOB APH_COMMON_SRC ${APH_ENGINE_COMMON_DIR}/*.cpp)

aph_setup_target(common ${APH_COMMON_SRC})

target_compile_definitions(aph-common
  PUBLIC
  $<$<BOOL:${APH_ENABLE_TRACING}>:APH_ENABLE_TRACY>
  $<$<BOOL:${APH_ENABLE_TRACING}>:TRACY_ENABLE>
)

target_link_libraries(aph-common
  PUBLIC
  unordered_dense::unordered_dense
  tomlplusplus::tomlplusplus
  $<$<BOOL:${APH_ENABLE_TRACING}>:Tracy::TracyClient>
  PRIVATE
  Backward::Interface
  dl
  dw
)
