file (GLOB MATH_SRC *.cpp)
aph_setup_target (math ${MATH_SRC})
target_link_libraries (
    aph-math
    PUBLIC glm
)
