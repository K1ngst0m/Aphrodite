cmake_minimum_required (VERSION 3.24)

project (Aphrodite)

set (
    APH_ROOT_DIR
    ${CMAKE_SOURCE_DIR}
)
set (
    CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH} "${APH_ROOT_DIR}/cmake"
)
list (APPEND CMAKE_MODULE_PATH "${APH_ROOT_DIR}/cmake/modules")

include (AphHelper)

aph_option (APH_SHARED "Enable building shared library" OFF)
aph_option (APH_ENABLE_TESTING "Enable testing" OFF)
aph_option (APH_ENABLE_TRACING "Enable tracer" OFF)
aph_option (APH_ENABLE_TSAN "Enable thread sanitizer" OFF)
aph_option (APH_ENABLE_ASAN "Enable address sanitizer" OFF)
aph_option (APH_ENABLE_MSAN "Enable memory sanitizer" OFF)

aph_option (APH_WSI_BACKEND "WSI backend (possible values: Auto, SDL)" "Auto" Auto SDL)

include (AphDefinitions)
include (AphCompilerOptions)
include (AphExternal)
include (IncludeSorter)
include (CMakeFormatter)
include (DependencyGrapher)

set (
    CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG
    "${APH_OUTPUT_DIR}/debug"
)
set (
    CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE
    "${APH_OUTPUT_DIR}/release"
)

add_subdirectory (src)
add_subdirectory (examples)

if (APH_ENABLE_TESTING)
    add_subdirectory (tests)
endif ()

# Add global include check targets
aph_add_global_include_check_targets ()

# Add CMake formatting targets
aph_add_cmake_format_targets ()

# Add dependency graph target
aph_add_dependency_graph_target ()
