file(GLOB API_EXCEPTION_SRC *.cpp)
aph_setup_target(exception ${API_EXCEPTION_SRC})

# Define compilation flags for backward-cpp
# target_compile_definitions(aph-exception PRIVATE
#     BACKWARD_HAS_DW=1
#     BACKWARD_HAS_UNWIND=1
#     BACKWARD_HAS_BACKTRACE_SYMBOL=1
# )

# Link against required libraries
target_link_libraries(
    aph-exception
    PRIVATE aph-common aph-allocator Backward::Interface
)
