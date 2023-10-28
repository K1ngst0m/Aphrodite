file(GLOB API_APP_SRC ${APH_ENGINE_APP_DIR}/*.cpp)
add_library(app STATIC ${API_APP_SRC})
aph_compiler_options(app)
target_include_directories(app PRIVATE ${APH_ENGINE_DIR})
