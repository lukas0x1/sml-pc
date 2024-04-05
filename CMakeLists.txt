cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(canvas VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

file(GLOB CPP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp )

add_library(
	POWRPROF
	SHARED
	${CPP_SRC}
	include/imgui/imgui.cpp
	include/imgui/imgui_demo.cpp
	include/imgui/imgui_draw.cpp
	include/imgui/imgui_impl_vulkan.cpp
	include/imgui/imgui_impl_win32.cpp
	include/imgui/imgui_tables.cpp
	include/imgui/imgui_widgets.cpp
	)



set(LIBMEM_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/release/libmem.lib)
set(CPR_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/release/cpr.lib)
set(CURL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/release/libcurl.lib)
set(COPY_TO ${CMAKE_CURRENT_SOURCE_DIR}/mod-demo/powrprof/release/POWRPROF.lib)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MDd -D_ITERATOR_DEBUG_LEVEL=2")
	set(CPR_PATH ${CMAKE_CURRENT_SOURCE_DIR}/debug/release/cpr.lib)
	set(CURL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/debug/release/libcurl.lib)
	set(LIBMEM_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/debug/libmem.lib)
	set(COPY_TO ${CMAKE_CURRENT_SOURCE_DIR}/mod-demo/powrprof/debug/POWRPROF.lib)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MD -D_ITERATOR_DEBUG_LEVEL=0")
endif()

find_library(VULKAN_LIBRARY NAMES vulkan-1 vulkan PATHS $ENV{VULKAN_SDK}/Lib)
find_path(VULKAN_INCLUDE_DIR NAMES vulkan/vulkan.h PATHS $ENV{VULKAN_SDK}/Include)


include_directories(
	include
	include/imgui
	include/libmem
)

target_compile_features(POWRPROF PRIVATE cxx_std_17)

target_include_directories(POWRPROF PRIVATE ${VULKAN_INCLUDE_DIR})

target_link_libraries(
	POWRPROF
	${LIBMEM_PATH}
	${CPR_PATH}
	${CURL_PATH}
	${VULKAN_LIBRARY}
)


add_custom_command(
	TARGET POWRPROF POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_LINKER_FILE:POWRPROF> ${COPY_TO}
)

add_custom_target(BeforeBuild DEPENDS ${COPY_TO})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/mod-demo)