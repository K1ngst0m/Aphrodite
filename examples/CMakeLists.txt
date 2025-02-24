# Function for building single example

function(buildExample EXAMPLE_PATH)
    SET(EXAMPLE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_PATH})
    message(STATUS "Generating project file for example in ${EXAMPLE_FOLDER}")

    get_filename_component(EXAMPLE_NAME ${EXAMPLE_PATH} NAME)

    # Main
    file(GLOB SOURCE *.cpp ${BASE_HEADERS} ${EXAMPLE_FOLDER}/*.cpp)
    SET(MAIN_CPP ${EXAMPLE_FOLDER}/${EXAMPLE_NAME}.cpp)
    if(EXISTS ${EXAMPLE_FOLDER}/main.cpp)
        SET(MAIN_CPP ${EXAMPLE_FOLDER}/main.cpp)
    ENDIF()
    if(EXISTS ${EXAMPLE_FOLDER}/${EXAMPLE_NAME}.h)
        SET(MAIN_HEADER ${EXAMPLE_FOLDER}/${EXAMPLE_NAME}.h)
    ENDIF()

    add_executable(${EXAMPLE_NAME} ${MAIN_CPP} ${SOURCE} ${MAIN_HEADER})
    aph_compiler_options(${EXAMPLE_NAME})
    target_link_libraries(${EXAMPLE_NAME} PRIVATE aphrodite::engine)
endfunction(buildExample)

# Build all examples
function(buildExamples)
    foreach(EXAMPLE ${EXAMPLES})
        buildExample(${EXAMPLE})
    endforeach(EXAMPLE)
endfunction(buildExamples)

set(EXAMPLES
    triangle_demo
    basic_texture
    mesh_shading
    # scene_manager
)

buildExamples()
