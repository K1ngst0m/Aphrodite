file (GLOB APH_MATH_SRC *.cpp)
aph_setup_target (math ${APH_MATH_SRC})
target_link_libraries (aph-math PUBLIC glm)
