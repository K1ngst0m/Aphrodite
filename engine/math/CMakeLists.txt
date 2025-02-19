file(GLOB APH_MATH_SRC ${APH_ENGINE_MATH_DIR}/*.cpp)
aph_setup_target(math ${APH_MATH_SRC})
target_include_directories(aph-math SYSTEM PUBLIC ${glm_SOURCE_DIR})
