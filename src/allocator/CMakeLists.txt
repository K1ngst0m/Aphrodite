file (GLOB APH_ALLOCATOR_SRC *.cpp)
aph_setup_target (allocator ${APH_ALLOCATOR_SRC})
target_link_libraries (
    aph-allocator
    PRIVATE aph-common aph-global
)
