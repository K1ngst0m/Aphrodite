file (GLOB API_THREADS_SRC *.cpp)
aph_setup_target (threads ${API_THREADS_SRC})
target_link_libraries (
    aph-threads
    PRIVATE aph-common aph-allocator
)
