file(GLOB APH_APP_SRC ${APH_ENGINE_APP_DIR}/*.cpp)
aph_setup_target(app ${APH_APP_SRC})
target_link_libraries(aph-app PRIVATE aph-common)
