file(GLOB APH_ALLOCATOR_SRC ${APH_ENGINE_ALLOCATOR_DIR}/*.cpp)
add_library(aph_allocator STATIC ${APH_ALLOCATOR_SRC})
aph_compiler_options(aph_allocator)
target_include_directories(aph_allocator PRIVATE
  ${APH_ENGINE_DIR}
)
