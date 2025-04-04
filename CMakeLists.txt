cmake_minimum_required (VERSION 3.24)

project (Aphrodite)

set (APH_ROOT_DIR ${CMAKE_SOURCE_DIR})
set (APH_BUILD_DIR ${CMAKE_BINARY_DIR})
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${APH_ROOT_DIR}/cmake")
set(APH_EXAMPLES_DIR ${APH_ROOT_DIR}/examples)
set(APH_SRC_DIR ${APH_ROOT_DIR}/src)
set(APH_OUTPUT_DIR ${APH_BUILD_DIR}/bin)

list (APPEND CMAKE_MODULE_PATH "${APH_ROOT_DIR}/cmake/modules")

include (AphHelper)

aph_option (APH_SHARED "Enable building shared library" OFF)
aph_option (APH_ENABLE_TESTING "Enable testing" OFF)
aph_option (APH_ENABLE_TRACING "Enable tracer" OFF)
aph_option (APH_ENABLE_TSAN "Enable thread sanitizer" OFF)
aph_option (APH_ENABLE_ASAN "Enable address sanitizer" OFF)
aph_option (APH_ENABLE_MSAN "Enable memory sanitizer" OFF)

aph_option (APH_WSI_BACKEND "WSI backend (possible values: Auto, SDL)" "Auto" Auto SDL)

include (AphCompilerOptions)
include (AphExternal)
include (CMakeFormatter)
include (ClangFormatter)
include (DependencyGrapher)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${APH_OUTPUT_DIR}")

add_subdirectory (src)
add_subdirectory (examples)

if (APH_ENABLE_TESTING)
    add_subdirectory (tests)
endif ()
