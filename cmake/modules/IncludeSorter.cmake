include_guard(GLOBAL)

# Include Sorter CMake Module
# This module provides targets for checking and fixing include ordering

function(aph_add_include_check_targets TARGET)
    # Find Python
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    # Get the compile_commands.json path
    get_target_property(compile_commands ${TARGET} COMPILE_COMMANDS)
    if(NOT compile_commands)
        set(compile_commands "${CMAKE_BINARY_DIR}/compile_commands.json")
    endif()

    # Add check-includes target
    add_custom_target(check-includes-${TARGET}
        COMMAND ${Python3_EXECUTABLE} 
            ${CMAKE_SOURCE_DIR}/scripts/include_sorter.py 
            --check 
            --compile-commands ${compile_commands}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Checking include ordering for ${TARGET}..."
    )

    # Add fix-includes target
    add_custom_target(fix-includes-${TARGET}
        COMMAND ${Python3_EXECUTABLE} 
            ${CMAKE_SOURCE_DIR}/scripts/include_sorter.py 
            --fix 
            --compile-commands ${compile_commands}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Fixing include ordering for ${TARGET}..."
    )

    # Add dependencies to ensure compile_commands.json is generated
    add_dependencies(check-includes-${TARGET} ${TARGET})
    add_dependencies(fix-includes-${TARGET} ${TARGET})
endfunction()

# Add global targets that check/fix all targets
# Find Python
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Get the compile_commands.json path
set(compile_commands "${CMAKE_BINARY_DIR}/compile_commands.json")

# Add global check-includes target
add_custom_target(check-includes
    COMMAND ${Python3_EXECUTABLE} 
        ${CMAKE_SOURCE_DIR}/scripts/include_sorter.py 
        --check 
        --compile-commands ${compile_commands}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Checking include ordering for all targets..."
)

# Add global fix-includes target
add_custom_target(fix-includes
    COMMAND ${Python3_EXECUTABLE} 
        ${CMAKE_SOURCE_DIR}/scripts/include_sorter.py 
        --fix 
        --compile-commands ${compile_commands}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Fixing include ordering for all targets..."
)

message(STATUS "Include sorter targets configured") 