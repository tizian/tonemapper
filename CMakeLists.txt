cmake_minimum_required (VERSION 3.13...3.18)
project(tonemapper)

## SETUP

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

## BOILERPLATE

# Enable C++17 mode on GCC / Clang
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
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
if(MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
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

# Build NanoGUI
set(NANOGUI_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_SHARED   OFF CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_PYTHON   OFF CACHE BOOL " " FORCE)
set(NANOGUI_BACKEND "OpenGL" CACHE STRING " " FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui ext_build/nanogui)
set_property(TARGET nanogui PROPERTY FOLDER "dependencies")
add_definitions(${NANOGUI_EXTRA_DEFS})

add_definitions(-DGL_SILENCE_DEPRECATION)

## INCLUDES

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui/include ${NANOGUI_EXTRA_INCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/tinyexr
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/tinyformat
)

## EXECUTABLE

add_executable(tonemapper
    "${PROJECT_SOURCE_DIR}/src/Gui.cpp"
    "${PROJECT_SOURCE_DIR}/src/Image.cpp"
    "${PROJECT_SOURCE_DIR}/src/Tonemap.cpp"

    "${PROJECT_SOURCE_DIR}/src/operators/LinearOperator.cpp"
    "${PROJECT_SOURCE_DIR}/src/operators/SrgbOperator.cpp"

    "${PROJECT_SOURCE_DIR}/src/main.cpp"
)

target_link_libraries(tonemapper nanogui ${NANOGUI_EXTRA_LIBS})
