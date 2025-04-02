# CMake Dependency Grapher Module
# This module provides targets for generating dependency graphs of the project

function(aph_add_dependency_graph_target)
    # Find Python
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    # Add target for generating DOT file
    add_custom_target(cmake-dependency-graph-dot
        COMMAND ${Python3_EXECUTABLE} 
            ${CMAKE_SOURCE_DIR}/scripts/dependency_grapher.py 
            ${CMAKE_SOURCE_DIR}/src
            -o ${CMAKE_BINARY_DIR}/cmake_dependency_graph.dot
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating CMake dependency graph DOT file..."
    )

    # Check if Graphviz is available
    find_program(DOT_EXECUTABLE dot)
    if(DOT_EXECUTABLE)
        # Add target for generating SVG
        add_custom_target(cmake-dependency-graph
            COMMAND ${Python3_EXECUTABLE} 
                ${CMAKE_SOURCE_DIR}/scripts/dependency_grapher.py 
                ${CMAKE_SOURCE_DIR}/src
                -f svg
                -o ${CMAKE_BINARY_DIR}/cmake_dependency_graph.dot
                -i ${CMAKE_BINARY_DIR}/cmake_dependency_graph.svg
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Generating CMake dependency graph image..."
        )

        # Add target for generating SVG
        add_custom_target(cmake-dependency-graph-svg
            COMMAND ${Python3_EXECUTABLE} 
                ${CMAKE_SOURCE_DIR}/scripts/dependency_grapher.py 
                ${CMAKE_SOURCE_DIR}/src
                -o ${CMAKE_BINARY_DIR}/cmake_dependency_graph.dot
                -i ${CMAKE_BINARY_DIR}/cmake_dependency_graph.svg
                -f svg
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Generating CMake dependency graph SVG..."
        )
    else()
        message(STATUS "Graphviz 'dot' command not found. Only DOT file generation will be available.")
        
        # Add an informational target
        add_custom_target(cmake-dependency-graph
            COMMAND ${CMAKE_COMMAND} -E echo "Graphviz not found. Install Graphviz for image generation."
            COMMAND ${CMAKE_COMMAND} -E echo "Generated DOT file: ${CMAKE_BINARY_DIR}/cmake_dependency_graph.dot"
            DEPENDS cmake-dependency-graph-dot
        )
    endif()
    
    # Add alias targets for backward compatibility
    add_custom_target(dependency-graph DEPENDS cmake-dependency-graph)
    add_custom_target(dependency-graph-dot DEPENDS cmake-dependency-graph-dot)
    add_custom_target(dependency-graph-svg DEPENDS cmake-dependency-graph-svg)
endfunction()
