file(GLOB APH_APP_SRC ${APH_ENGINE_APP_DIR}/*.cpp)
add_library(app STATIC ${APH_APP_SRC})
aph_compiler_options(app)
target_link_libraries(app PRIVATE common)
target_include_directories(app PRIVATE
  ${APH_ENGINE_DIR}
)
