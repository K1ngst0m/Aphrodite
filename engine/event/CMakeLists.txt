file(GLOB APH_EVENT_SRC ${APH_ENGINE_EVENT_DIR}/*.cpp)
add_library(aph_event STATIC ${APH_EVENT_SRC})
aph_compiler_options(aph_event)
target_include_directories(aph_event PRIVATE
  ${APH_ENGINE_DIR}
  ${APH_EXTERNAL_DIR}/unordered_dense/include
)
