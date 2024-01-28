file(GLOB APH_COMMON_SRC ${APH_ENGINE_COMMON_DIR}/*.cpp)
add_library(common STATIC ${APH_COMMON_SRC})
aph_compiler_options(common)
target_include_directories(common PUBLIC
  ${APH_ENGINE_DIR}
  ${unordered_dense_SOURCE_DIR}/include

  PRIVATE
  ${APH_EXTERNAL_DIR}/backward-cpp/include
)

if (APH_ENABLE_TRACING)
  target_include_directories(common SYSTEM PUBLIC
    ${APH_EXTERNAL_DIR}/tracy/public
  )
  target_compile_definitions(common PRIVATE APH_ENABLE_TRACER)
endif()

target_link_libraries(common PUBLIC
  unordered_dense::unordered_dense
  Tracy::TracyClient
)
