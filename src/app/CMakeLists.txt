file (GLOB APH_APP_SRC *.cpp)
aph_setup_target (app ${APH_APP_SRC})
target_link_libraries (
    aph-app
    PRIVATE aph-common aph-global aph-exception
)
