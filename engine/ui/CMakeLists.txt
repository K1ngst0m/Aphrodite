file(GLOB APH_UI_SRC ${APH_ENGINE_UI_DIR}/*.cpp)
aph_setup_target(ui ${APH_UI_SRC})
target_link_libraries(aph-ui
  PUBLIC imgui
  PRIVATE aph-math
  )
