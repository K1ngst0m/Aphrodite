include_guard()

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT error)
if(NOT ipo_supported)
    message(WARNING "IPO/LTO not supported: <${error}>")
endif()

if (APH_ENABLE_TSAN)
  add_compile_options(-fsanitize=thread)
  add_link_options(-fsanitize=thread)
endif()
if (APH_ENABLE_ASAN)
  add_compile_options(-fsanitize=thread)
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif()
if (APH_ENABLE_MSAN)
  add_compile_options(-fsanitize=thread)
  add_compile_options(-fsanitize=memory)
  add_link_options(-fsanitize=memory)
endif()

# find linker
find_program(MOLD_LINKER mold)
# Set the linker
if(MOLD_LINKER)
    message(STATUS "Using mold as the linker")
    set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=mold")
    set(CMAKE_SHARED_LINKER_FLAGS "-fuse-ld=mold")
else()
    message(STATUS "Using system's default ld as the linker")
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
          -Wall -Wextra -Wshadow

          # TODO
          -Wno-non-virtual-dtor
          -Wno-sign-compare -Wcast-align -Wno-missing-field-initializers -Wno-unused-parameter
          -Wno-cast-align -Wno-format-security -Wno-nullability-completeness
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
                # -O0 -ggdb -g # for debugging
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
