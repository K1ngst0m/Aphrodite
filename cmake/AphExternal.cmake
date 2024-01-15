# spirv-headers
option(SPIRV_HEADERS_SKIP_INSTALL "" ON)
option(SPIRV_HEADERS_SKIP_EXAMPLES "" ON)

# spirv-tools
option(SKIP_SPIRV_TOOLS_INSTALL "" ON)

# spirv-cross
option(SPIRV_CROSS_CLI "" OFF)
option(SPIRV_CROSS_ENABLE_TESTS "" OFF)

# glslang
option(ENABLE_GLSLANG_INSTALL "" OFF)
option(ENABLE_GLSLANG_BINARIES "" OFF)
option(SKIP_GLSLANG_INSTALL "" ON)

# mimalloc
option(MI_BUILD_SHARED "" OFF)
option(MI_BUILD_OBJECT "" OFF)
option(MI_BUILD_STATIC "" ON)
option(MI_OVERRIDE "" ON)

# glfw
option(GLFW_BUILD_DOCS "" OFF)
option(GLFW_BUILD_TESTS "" OFF)
option(GLFW_BUILD_EXAMPLES "" OFF)
option(BUILD_SHARED_LIBS "" OFF)


add_subdirectory(${APH_EXTERNAL_DIR})
add_subdirectory(${APH_EXTERNAL_DIR}/mimalloc EXCLUDE_FROM_ALL)
add_subdirectory(${APH_EXTERNAL_DIR}/vma EXCLUDE_FROM_ALL)
add_subdirectory(${APH_EXTERNAL_DIR}/volk EXCLUDE_FROM_ALL)
add_subdirectory(${APH_EXTERNAL_DIR}/spirv-cross EXCLUDE_FROM_ALL)
add_subdirectory(${APH_EXTERNAL_DIR}/imgui EXCLUDE_FROM_ALL)
add_subdirectory(${APH_EXTERNAL_DIR}/slang EXCLUDE_FROM_ALL)
add_subdirectory(${APH_EXTERNAL_DIR}/tinygltf EXCLUDE_FROM_ALL)
add_subdirectory(${APH_EXTERNAL_DIR}/glm EXCLUDE_FROM_ALL)
add_subdirectory(${APH_EXTERNAL_DIR}/tracy EXCLUDE_FROM_ALL)
add_subdirectory(${APH_EXTERNAL_DIR}/unordered_dense EXCLUDE_FROM_ALL)

find_package(PkgConfig REQUIRED)
pkg_check_modules(xcb REQUIRED IMPORTED_TARGET xcb)

# wsi backend
set(VALID_WSI_BACKENDS Auto GLFW SDL2)
if(NOT (APH_WSI_BACKEND IN_LIST VALID_WSI_BACKENDS))
    message(FATAL_ERROR "Wrong value passed for APH_WSI_BACKEND, use one of: Auto, GLFW, SDL2")
endif()

if(APH_WSI_BACKEND STREQUAL "Auto" OR APH_WSI_BACKEND STREQUAL "GLFW")
    find_package(GLFW3)
    if(NOT GLFW3_FOUND)
        message(FATAL_ERROR "GLFW3 library not found!")
    endif()
elseif(APH_WSI_BACKEND STREQUAL "SDL2")
    find_package(SDL2 CONFIG REQUIRED)
    if(NOT SDL2_FOUND)
        message(FATAL_ERROR "SDL2 library not found!")
    endif()
endif()
