file(GLOB API_CLI_SRC ${APH_ENGINE_CLI_DIR}/*.cpp)
aph_setup_target(cli ${API_CLI_SRC})
target_link_libraries(aph-cli PRIVATE aph-common)
