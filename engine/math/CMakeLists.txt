file(GLOB APH_MATH_SRC ${APH_ENGINE_MATH_DIR}/*.cpp)
add_library(aph_math STATIC ${APH_MATH_SRC})
aph_compiler_options(aph_math)
target_include_directories(aph_math PUBLIC ${glm_SOURCE_DIR})
