file(GLOB APH_MODULE_SRC ${APH_ENGINE_MODULE_DIR}/*.cpp)
add_library(aph_module STATIC ${APH_MODULE_SRC})
aph_compiler_options(aph_module)
target_include_directories(aph_module PRIVATE ${APH_ENGINE_DIR})
