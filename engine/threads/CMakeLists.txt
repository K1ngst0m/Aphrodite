file(GLOB API_THREADS_SRC ${APH_ENGINE_THREADS_DIR}/*.cpp)
add_library(threads STATIC ${API_THREADS_SRC})
aph_compiler_options(threads)
target_include_directories(threads PRIVATE
  ${APH_ENGINE_DIR}
  ${APH_EXTERNAL_DIR}/unordered_dense/include
)
target_link_libraries(threads PRIVATE common aph_allocator)
