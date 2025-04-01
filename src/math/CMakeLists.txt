file (GLOB APH_MATH_SRC *.cpp)
aph_setup_target (math ${APH_MATH_SRC})
target_include_directories (
    aph-math SYSTEM
    PUBLIC ${glm_SOURCE_DIR}
)
