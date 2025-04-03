include_guard(GLOBAL)

# CMake Formatter Module
# This module provides targets for checking and fixing CMake formatting

# Find Python
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Add check-cmake-format target
add_custom_target(aph-check-cmake-format
    COMMAND ${Python3_EXECUTABLE} 
        ${CMAKE_SOURCE_DIR}/scripts/cmake_formatter.py 
        --check
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Checking CMake formatting..."
)

# Add fix-cmake-format target
add_custom_target(aph-fix-cmake-format
    COMMAND ${Python3_EXECUTABLE} 
        ${CMAKE_SOURCE_DIR}/scripts/cmake_formatter.py 
        --fix
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Fixing CMake formatting..."
)

message(STATUS "CMake formatter targets configured") 