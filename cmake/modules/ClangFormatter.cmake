# Clang Formatter Module
# This module provides targets for checking and fixing C++ code formatting

function(aph_add_clang_format_targets)
    # Find Python
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    # Add check-clang-format target
    add_custom_target(aph-check-clang-format
        COMMAND ${Python3_EXECUTABLE} 
            ${CMAKE_SOURCE_DIR}/scripts/clang_formatter.py 
            --check
            --compile-commands ${CMAKE_BINARY_DIR}/compile_commands.json
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Checking C++ formatting..."
    )

    # Add check-clang-format-debug target with verbose output
    add_custom_target(aph-check-clang-format-debug
        COMMAND ${Python3_EXECUTABLE} 
            ${CMAKE_SOURCE_DIR}/scripts/clang_formatter.py 
            --check
            --compile-commands ${CMAKE_BINARY_DIR}/compile_commands.json
            --debug
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Checking C++ formatting with debug output..."
    )

    # Add fix-clang-format target
    add_custom_target(aph-fix-clang-format
        COMMAND ${Python3_EXECUTABLE} 
            ${CMAKE_SOURCE_DIR}/scripts/clang_formatter.py 
            --fix
            --compile-commands ${CMAKE_BINARY_DIR}/compile_commands.json
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Fixing C++ formatting..."
    )

    # Add fix-clang-format-debug target with verbose output
    add_custom_target(aph-fix-clang-format-debug
        COMMAND ${Python3_EXECUTABLE} 
            ${CMAKE_SOURCE_DIR}/scripts/clang_formatter.py 
            --fix
            --compile-commands ${CMAKE_BINARY_DIR}/compile_commands.json
            --debug
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Fixing C++ formatting with debug output..."
    )
endfunction() 