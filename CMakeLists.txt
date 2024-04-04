cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)



project(canvas VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(VULKAN_SDK "C:/VulkanSDK/1.3.280.0")

add_library(
	POWRPROF
	SHARED
	main.cpp
	menu.cpp
	vulkan_hooks.cpp
	include/imgui/imgui.cpp
	include/imgui/imgui_demo.cpp
	include/imgui/imgui_draw.cpp
	include/imgui/imgui_impl_vulkan.cpp
	include/imgui/imgui_impl_win32.cpp
	include/imgui/imgui_tables.cpp
	include/imgui/imgui_widgets.cpp
	)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MDd -D_ITERATOR_DEBUG_LEVEL=2")

find_library(VULKAN_LIBRARY NAMES vulkan-1 vulkan PATHS ${VULKAN_SDK}/Lib)
find_path(VULKAN_INCLUDE_DIR NAMES vulkan/vulkan.h PATHS ${VULKAN_SDK}/Include)

target_compile_features(POWRPROF PRIVATE cxx_std_17)
target_include_directories(POWRPROF PRIVATE ${VULKAN_INCLUDE_DIR})

target_link_libraries(
	POWRPROF
	${CMAKE_CURRENT_SOURCE_DIR}/lib/libmem.lib
	${VULKAN_LIBRARY}
	)