cmake_minimum_required(VERSION 3.10)

# preferred gcc versions
set(GCC_VERSIONS 13 12 11)

set(FOUND_GCC FALSE)
foreach(VERS ${GCC_VERSIONS})
    find_program(GCC_BIN NAMES gcc-${VERS})
    find_program(GPP_BIN NAMES g++-${VERS})
    if(GCC_BIN AND GPP_BIN)
        message(STATUS "Found gcc-${VERS} in PATH; using it as compiler.")
        set(CMAKE_C_COMPILER "${GCC_BIN}" CACHE FILEPATH "C compiler" FORCE)
        set(CMAKE_CXX_COMPILER "${GPP_BIN}" CACHE FILEPATH "C++ compiler" FORCE)
        set(FOUND_GCC TRUE)
        break()
    endif()
endforeach()

# try unversioned gcc/g++
if(NOT FOUND_GCC)
    find_program(GCC_BIN NAMES gcc)
    find_program(GPP_BIN NAMES g++)
    if(GCC_BIN AND GPP_BIN)
        message(STATUS "Found gcc in PATH; using it as compiler.")
        set(CMAKE_C_COMPILER "${GCC_BIN}" CACHE FILEPATH "C compiler" FORCE)
        set(CMAKE_CXX_COMPILER "${GPP_BIN}" CACHE FILEPATH "C++ compiler" FORCE)
        set(FOUND_GCC TRUE)
    endif()
endif()

if(NOT FOUND_GCC)
    message(FATAL_ERROR "No usable GCC found on your system. Aborting.")
endif()
