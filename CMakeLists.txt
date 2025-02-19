cmake_minimum_required(VERSION 3.24)

project(Aphrodite)

set(APH_ROOT_DIR ${CMAKE_SOURCE_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${APH_ROOT_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${APH_ROOT_DIR}/cmake/modules")

include(AphHelper)

aph_option(APH_SHARED "Enable building shared library" OFF)
aph_option(APH_ENABLE_TESTING "Enable testing" OFF)
aph_option(APH_ENABLE_TRACING "Enable tracer" OFF)
aph_option(APH_ENABLE_TSAN "Enable thread sanitizer" OFF)
aph_option(APH_ENABLE_ASAN "Enable address sanitizer" ON)
aph_option(APH_ENABLE_MSAN "Enable memory sanitizer" OFF)

aph_option(APH_WSI_BACKEND "WSI backend (possible values: Auto, GLFW, SDL2)" "Auto" Auto GLFW SDL2)

include(AphDefinitions)
include(AphCompilerOptions)
include(AphExternal)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${APH_OUTPUT_DIR}/debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${APH_OUTPUT_DIR}/release")

add_subdirectory(engine)
add_subdirectory(examples)

if (APH_ENABLE_TESTING)
    add_subdirectory(tests)
endif()
