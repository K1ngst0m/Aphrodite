file(GLOB APH_ALLOCATOR_SRC ${APH_ENGINE_ALLOCATOR_DIR}/*.cpp)
aph_setup_target(allocator ${APH_ALLOCATOR_SRC})
target_link_libraries(aph-allocator PUBLIC
  aph-common
  # mimalloc-static
)
