cmake_minimum_required (VERSION 3.13...3.18)

set(YEAR    "2022")
set(VERSION "2.0.0")
add_definitions(-D YEAR=\\""${YEAR}"\\")
add_definitions(-D VERSION=\\""${VERSION}"\\")

message(STATUS "tonemapper v${VERSION}")

if (APPLE)
    execute_process(COMMAND uname -m OUTPUT_VARIABLE TONEMAPPER_ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

project(tonemapper)

if (APPLE AND CMAKE_OSX_ARCHITECTURES STREQUAL "")
    set(CMAKE_OSX_ARCHITECTURES ${TONEMAPPER_ARCH})
endif()

if (APPLE)
    message(STATUS "tonemapper: using macOS architecture ${CMAKE_OSX_ARCHITECTURES}")
endif()

option(TONEMAPPER_BUILD_GUI    "Build the tonemapping GUI?"    ON)
option(TONEMAPPER_MACOS_BUNDLE "Create a .app bundle on macOS" ON)

if (TONEMAPPER_MACOS_BUNDLE AND NOT TONEMAPPER_BUILD_GUI)
    set(TONEMAPPER_BUILD_GUI ON)
endif()

if (TONEMAPPER_BUILD_GUI)
    message(STATUS "tonemapper: build GUI")
endif()

if (APPLE AND TONEMAPPER_MACOS_BUNDLE)
    message(STATUS "tonemapper: build macOS .app bundle")
endif()

## SETUP

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

## BOILERPLATE

# Enable C++17 mode on GCC / Clang
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
elseif (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
endif()

include(CheckCXXCompilerFlag)
include(CheckCXXSourceRuns)

macro(CHECK_CXX_COMPILER_AND_LINKER_FLAGS _RESULT _CXX_FLAGS _LINKER_FLAGS)
    set(CMAKE_REQUIRED_FLAGS ${_CXX_FLAGS})
    set(CMAKE_REQUIRED_LIBRARIES ${_LINKER_FLAGS})
    set(CMAKE_REQUIRED_QUIET TRUE)
    check_cxx_source_runs("#include <iostream>\nint main(int argc, char **argv) { std::cout << \"test\"; return 0; }" ${_RESULT})
    set(CMAKE_REQUIRED_FLAGS "")
    set(CMAKE_REQUIRED_LIBRARIES "")
endmacro()

# Compiler warnings
if (MSVC)
    if (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-anonymous-struct -Wno-c99-extensions -Wno-nested-anon-types")
    endif()
endif()

# Prefer libc++ in conjunction with Clang
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if (CMAKE_CXX_FLAGS MATCHES "-stdlib=libc\\+\\+")
        message(STATUS "tonemapper: using libc++.")
    else()
        CHECK_CXX_COMPILER_AND_LINKER_FLAGS(HAS_LIBCPP "-stdlib=libc++" "-stdlib=libc++")
        if (HAS_LIBCPP)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
            set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
            message(STATUS "tonemapper: using libc++.")
        else()
            message(STATUS "tonemapper: NOT using libc++.")
      endif()
    endif()
endif()

## DEPENDENCIES

if (TONEMAPPER_BUILD_GUI)
    add_definitions(-DTONEMAPPER_BUILD_GUI)

    if (NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui/ext/glfw")
    message(FATAL_ERROR "Dependency repositories (NanoGUI, GLFW, etc.) are missing! "
        "You probably did not clone the project with --recursive. It is possible to recover by calling \"git submodule update --init --recursive\"")
    endif()

    # Build NanoGUI
    set(NANOGUI_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
    set(NANOGUI_BUILD_SHARED   OFF CACHE BOOL " " FORCE)
    set(NANOGUI_BUILD_PYTHON   OFF CACHE BOOL " " FORCE)
    set(NANOGUI_BACKEND "OpenGL" CACHE STRING " " FORCE)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui ext_build/nanogui)
    set_property(TARGET nanogui PROPERTY FOLDER "dependencies")
    add_definitions(${NANOGUI_EXTRA_DEFS})

    add_compile_options(${NANOGUI_NATIVE_FLAGS})
    add_definitions(-DGL_SILENCE_DEPRECATION)
endif()

## INCLUDES

set(TONEMAPPER_INCLUDE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/tinyexr
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/tinyformat
)
if (TONEMAPPER_BUILD_GUI)
    set(TONEMAPPER_INCLUDE_FILES
        ${TONEMAPPER_INCLUDE_FILES}
        ${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui/include
        ${NANOGUI_EXTRA_INCS}
    )
endif()

include_directories(
    ${TONEMAPPER_INCLUDE_FILES}
)

## EXECUTABLE

set(TONEMAPPER_SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/src/main.cpp
    ${PROJECT_SOURCE_DIR}/src/Image.cpp
    ${PROJECT_SOURCE_DIR}/src/Tonemap.cpp
)
if (TONEMAPPER_BUILD_GUI)
    set(TONEMAPPER_SOURCE_FILES
        ${TONEMAPPER_SOURCE_FILES}
        ${PROJECT_SOURCE_DIR}/src/Gui.cpp
    )
endif()

file(GLOB_RECURSE TONEMAPPER_OPERATOR_FILES
    "${PROJECT_SOURCE_DIR}/src/operators/*.cpp"
)

if (APPLE AND TONEMAPPER_MACOS_BUNDLE)
    set(MACOSX_BUNDLE_ICON_FILE tonemapper.icns)
    set(MACOSX_BUNDLE_BUNDLE_NAME "tonemapper")
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${VERSION})
    set(MACOSX_BUNDLE_COPYRIGHT "© 2022 Tizian Zeltner")
    set(macOSIcon ${CMAKE_CURRENT_SOURCE_DIR}/res/tonemapper.icns)
    set_source_files_properties(${macOSIcon} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    add_executable(tonemapper MACOSX_BUNDLE
        ${macOSIcon}
        ${TONEMAPPER_SOURCE_FILES}
        ${TONEMAPPER_OPERATOR_FILES}
    )
elseif (MSVC)
    set(rcFile ${CMAKE_CURRENT_SOURCE_DIR}/res/tonemapper.rc)
    add_executable(tonemapper
        ${rcFile}
        ${TONEMAPPER_SOURCE_FILES}
        ${TONEMAPPER_OPERATOR_FILES}
    )
else()
    add_executable(tonemapper
        ${TONEMAPPER_SOURCE_FILES}
        ${TONEMAPPER_OPERATOR_FILES}
    )
endif()

if (TONEMAPPER_BUILD_GUI)
    target_link_libraries(tonemapper nanogui ${NANOGUI_EXTRA_LIBS})
endif()
