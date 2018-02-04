cmake_minimum_required(VERSION 2.6)

project(tonemapper)

set(CMAKE_BUILD_TYPE Release)

if(MSVC)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++ -Wall")
endif()

set(NANOGUI_BUILD_EXAMPLE OFF CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_SHARED  OFF CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_PYTHON  OFF CACHE BOOL " " FORCE)


include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui/ext/eigen
	${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui/ext/glfw/include
	${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui/ext/glew/include
	${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui/ext/nanovg/src
	${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui/include
	${CMAKE_CURRENT_SOURCE_DIR}/ext/filesystem
	${CMAKE_CURRENT_SOURCE_DIR}/ext/nanovg
	${CMAKE_CURRENT_SOURCE_DIR}/ext/tinyexr
	${CMAKE_CURRENT_SOURCE_DIR}/ext/stb
	${CMAKE_CURRENT_SOURCE_DIR}/ext
	${CMAKE_CURRENT_SOURCE_DIR}/src
)

set (EXTRA_SOURCE "")
if (WIN32)
	set(EXTRA_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/res/tonemapper.rc)
elseif(APPLE)
	set(EXTRA_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/res/tonemapper.icns)
endif()
source_group(icons FILES ${EXTRA_SOURCE})


file(GLOB NANOGUI_SRC_FILES ${PROJECT_SOURCE_DIR}/ext/nanogui/src/*.cpp)
source_group(nanogui FILES 
    ${NANOGUI_SRC_FILES}
    ${PROJECT_SOURCE_DIR}/ext/nanovg/nanovg.c
    ${PROJECT_SOURCE_DIR}/src/glad.c
    )

set(GLFW_SRC        
    ${PROJECT_SOURCE_DIR}/ext/GLFW/src/win32_init.c 
    ${PROJECT_SOURCE_DIR}/ext/GLFW/src/win32_joystick.c
    ${PROJECT_SOURCE_DIR}/ext/GLFW/src/win32_monitor.c 
    ${PROJECT_SOURCE_DIR}/ext/GLFW/src/win32_time.c 
    ${PROJECT_SOURCE_DIR}/ext/GLFW/src/win32_tls.c 
    ${PROJECT_SOURCE_DIR}/ext/GLFW/src/win32_window.c
    ${PROJECT_SOURCE_DIR}/ext/GLFW/src/wgl_context.c 
    ${PROJECT_SOURCE_DIR}/ext/GLFW/src/egl_context.c
    ${PROJECT_SOURCE_DIR}/ext/GLFW/src/context.c 
    ${PROJECT_SOURCE_DIR}/ext/GLFW/src/init.c 
    ${PROJECT_SOURCE_DIR}/ext/GLFW/src/input.c 
    ${PROJECT_SOURCE_DIR}/ext/GLFW/src/monitor.c 
    ${PROJECT_SOURCE_DIR}/ext/GLFW/src/vulkan.c 
    ${PROJECT_SOURCE_DIR}/ext/GLFW/src/window.c
)
source_group(glfw FILES ${GLFW_SRC})


file(GLOB OPERATORS_SRC "src/operators/*")
source_group(operators FILES ${OPERATORS_SRC})

add_executable(tonemapper MACOSX_BUNDLE
	src/image.cpp
	src/gui.cpp
	src/main.cpp
    src/glad.c
    src/global.h
    src/gui.h
    src/image.h
    src/tonemap.h
    src/color.h
    ext/nanovg/nanovg.c
    ${OPERATORS_SRC}
    ${NANOGUI_SRC_FILES}
    ${GLFW_SRC}
	${EXTRA_SOURCE}
)

if (WIN32)
    target_compile_definitions(tonemapper PRIVATE _GLFW_WIN32 NANOGUI_GLAD)
endif()
#target_link_libraries(tonemapper ${CMAKE_CURRENT_SOURCE_DIR}/lib/nanogui.lib)

set_target_properties(tonemapper PROPERTIES OUTPUT_NAME "Tone Mapper")

if (APPLE)
	set_target_properties(tonemapper PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/res/info.plist)
	set_source_files_properties(res/tonemapper.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()
