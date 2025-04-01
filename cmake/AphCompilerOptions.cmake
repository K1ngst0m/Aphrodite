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
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
  add_compile_definitions(MI_TRACK_ASAN=ON)
endif()
if (APH_ENABLE_MSAN)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-fsanitize=memory -fPIE)
    add_link_options(-fsanitize=memory -fPIE -pie)
  else()
    message(WARNING "Memory sanitizer is only supported by clang.")
  endif()
endif()

if(MSVC)
    add_compile_options(/GR- /EHs-c-)
else()
    add_compile_options(-fno-rtti -fno-exceptions)
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

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    add_compile_options(-fPIC)
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

          -Wno-sign-compare -Wcast-align -Wno-missing-field-initializers -Wno-unused-parameter
          -Wno-cast-align -Wno-format-security -Wno-nullability-completeness

          -Wno-unused-but-set-parameter
        )

        target_precompile_headers(${TARGET} PRIVATE ${APH_SRC_DIR}/pch.h)

        # common options
        target_compile_options(${TARGET} PRIVATE
            -fdiagnostics-color=always
            -mavx2
        )

        target_link_options(${TARGET} PRIVATE
        )

        # build type specific
        target_compile_options(${TARGET} PRIVATE
            $<$<CONFIG:Debug>:
                -O0 -ggdb -g
                -fno-omit-frame-pointer
            >

            $<$<CONFIG:Release>:
                # -O0 -ggdb -g # for debugging
                -O3
                -march=native
                -flto
            >
        )

        target_link_options(${TARGET} PRIVATE
            $<$<CONFIG:Release>:
                -flto
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
