file (GLOB APH_EVENT_SRC *.cpp)
aph_setup_target (event ${APH_EVENT_SRC})
target_link_libraries (
    aph-event
    PRIVATE aph-common
)
