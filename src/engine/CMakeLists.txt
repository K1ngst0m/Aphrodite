file (GLOB APH_ENGINE_SRC *.cpp)

# Remove debug.cpp in Release mode
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    list(REMOVE_ITEM APH_ENGINE_SRC debug.cpp)
endif()

aph_setup_target (engine ${APH_ENGINE_SRC})
target_link_libraries (
    aph-engine
    PRIVATE aph-common aph-api imgui aph-math
)
