include_guard()

function(aph_compiler_options TARGET)
    set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF

        C_STANDARD 11
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF

        POSITION_INDEPENDENT_CODE TRUE
    )

    target_compile_features(${TARGET} PUBLIC cxx_std_17)
    target_compile_features(${TARGET} PUBLIC c_std_11)

    # Compiler flags for GCC/Clang
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")

        # setup target compile warning
        target_compile_options(${TARGET} PRIVATE
          -Wall

          -Wno-sign-compare
        )

        # common options
        target_compile_options(${TARGET} PRIVATE
            -fdiagnostics-color=auto
        )

        target_link_options(${TARGET} PRIVATE
        )

        # build type specific
        target_compile_options(${TARGET} PRIVATE
            $<$<CONFIG:Debug>:
                -O0 -ggdb -g
            >

            $<$<CONFIG:Release>:
                -O3
            >
        )

        target_compile_definitions(${TARGET} PRIVATE
            $<$<CONFIG:Debug>:
            APH_DEBUG
            >

            $<$<CONFIG:Release>:
                # NDEBUG
            >
        )

    else ()
        message(FATAL_ERROR "MB_COMPILER is not valid")
    endif ()

endfunction()
