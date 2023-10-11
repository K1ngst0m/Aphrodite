# Function for building single example
include_directories(../engine)

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

    # Add shaders
    set(SHADER_DIR_GLSL "../data/shaders/glsl/${EXAMPLE_PATH}")
    file(GLOB SHADERS_GLSL "${SHADER_DIR_GLSL}/*.vert" "${SHADER_DIR_GLSL}/*.frag" "${SHADER_DIR_GLSL}/*.comp" "${SHADER_DIR_GLSL}/*.geom" "${SHADER_DIR_GLSL}/*.tesc" "${SHADER_DIR_GLSL}/*.tese" "${SHADER_DIR_GLSL}/*.mesh" "${SHADER_DIR_GLSL}/*.task" "${SHADER_DIR_GLSL}/*.rgen" "${SHADER_DIR_GLSL}/*.rchit" "${SHADER_DIR_GLSL}/*.rmiss" "${SHADER_DIR_GLSL}/*.rcall")

    add_executable(${EXAMPLE_NAME} ${MAIN_CPP} ${SOURCE} ${MAIN_HEADER} ${SHADERS_GLSL} ${SHADERS_HLSL} ${README_FILES})
    aph_compiler_options(${EXAMPLE_NAME})
    target_link_libraries(${EXAMPLE_NAME} engine)
endfunction(buildExample)

# Build all examples
function(buildExamples)
    foreach(EXAMPLE ${EXAMPLES})
        buildExample(${EXAMPLE})
    endforeach(EXAMPLE)
endfunction(buildExamples)

set(EXAMPLES
    triangle_demo
    base_texture
    # scene_manager
)

buildExamples()
