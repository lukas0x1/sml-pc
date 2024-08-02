cmake_minimum_required(VERSION 3.20.0 FATAL_ERROR)

project(canvas VERSION 0.1.0 LANGUAGES CXX C)

# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)     #uncomment if you wanna see compile commands

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD -D_ITERATOR_DEBUG_LEVEL=0")
endif()

file(GLOB CPP_SRC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui/*.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/include/cpr/*.cpp #only include when you need
)

add_library(
	canvas
	SHARED
	${CPP_SRC}
)


set(LIBS 
    #${CMAKE_CURRENT_SOURCE_DIR}/lib/release/libcurl.lib   #uncomment When Needed
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/release/libmem.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/vulkan.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/ktx.lib
	${CMAKE_CURRENT_SOURCE_DIR}/lib/release/minhook.lib
)
# set(COPY_TO ${CMAKE_CURRENT_SOURCE_DIR}/mod-demo/canvas/release/canvas.lib)   #mod demo is not used rn uncomment when needed
set(COPY_TO ${CMAKE_CURRENT_SOURCE_DIR}/That_Sky_Mod/lib/release/canvas.lib)      

include_directories(
	include
	include/imgui
	include/libmem
	include/minhook
)

target_compile_features(canvas PRIVATE cxx_std_20)

target_link_libraries(
	canvas
	ntdll
    ${LIBS}
)


add_custom_command(
	TARGET canvas POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_LINKER_FILE:canvas> ${COPY_TO}
)

# add_custom_target(BeforeBuild DEPENDS ${COPY_TO})

# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/mod-demo)