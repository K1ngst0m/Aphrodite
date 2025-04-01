file (GLOB API_CLI_SRC *.cpp)
aph_setup_target (cli ${API_CLI_SRC})
target_link_libraries (
    aph-cli
    PRIVATE aph-common
)
