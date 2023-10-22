include_guard()

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT error)
if(NOT ipo_supported)
    message(WARNING "IPO/LTO not supported: <${error}>")
endif()

function(aph_compiler_options TARGET)
    set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF

        C_STANDARD 11
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF

        POSITION_INDEPENDENT_CODE TRUE
        INTERPROCEDURAL_OPTIMIZATION $<${ipo_supported}:TRUE>
    )

    target_compile_features(${TARGET} PUBLIC cxx_std_20)
    target_compile_features(${TARGET} PUBLIC c_std_11)

    # Compiler flags for GCC/Clang
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")

        # setup target compile warning
        target_compile_options(${TARGET} PRIVATE
          -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic

          -Wno-sign-compare -Wcast-align -Wno-missing-field-initializers -Wno-unused-parameter
        )

        target_precompile_headers(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/engine/pch.h)

        # common options
        target_compile_options(${TARGET} PRIVATE
            -fdiagnostics-color=auto

            -mavx2
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
