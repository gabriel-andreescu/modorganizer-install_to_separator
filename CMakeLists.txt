cmake_minimum_required(VERSION 3.22)

if (NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE FILEPATH "vcpkg toolchain file")
endif ()

set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "vcpkg target triplet")
set(VCPKG_MANIFEST_NO_DEFAULT_FEATURES ON CACHE BOOL "Use only default vcpkg manifest dependencies")

set(INSTALL_TO_SEPARATOR_RELEASE_VERSION "0.1.2" CACHE STRING "Install To Separator release version")

project(install_to_separator VERSION ${INSTALL_TO_SEPARATOR_RELEASE_VERSION})

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../../../install"
        CACHE PATH "Install prefix" FORCE)
endif ()

foreach (prefix
        "${CMAKE_CURRENT_LIST_DIR}/../../cmake_common"
        "${CMAKE_CURRENT_LIST_DIR}/../../../install/lib/cmake"
        "C:/Qt/6.7.3/msvc2019_64")
    if (EXISTS "${prefix}")
        list(APPEND CMAKE_PREFIX_PATH "${prefix}")
    endif ()
endforeach ()
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE STRING "Additional package prefixes")

find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy)

add_subdirectory(src)

if (CLANG_TIDY_EXECUTABLE)
    add_custom_target(install_to_separator_clang_tidy
        COMMAND "${CMAKE_COMMAND}"
            "-DCLANG_TIDY_EXECUTABLE=${CLANG_TIDY_EXECUTABLE}"
            "-DCOMPILE_COMMANDS_FILE=${CMAKE_BINARY_DIR}/compile_commands.json"
            "-DBUILD_DIR=${CMAKE_BINARY_DIR}"
            "-DSOURCE_ROOT=${CMAKE_CURRENT_LIST_DIR}/src"
            -P "${CMAKE_CURRENT_LIST_DIR}/cmake/run_clang_tidy.cmake"
        COMMENT "Running clang-tidy on install_to_separator source files"
        VERBATIM)
else ()
    add_custom_target(install_to_separator_clang_tidy
        COMMAND "${CMAKE_COMMAND}" -E false
        COMMENT "clang-tidy was not found")
endif ()
