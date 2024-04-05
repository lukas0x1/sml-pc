cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(mod-demo VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

file(GLOB MOD_DEMO_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp )

add_library(
	demo
	SHARED
	${MOD_DEMO_SRC}
)

set(POWRPROF_PATH ${CMAKE_CURRENT_SOURCE_DIR}/powrprof/release/POWRPROF.lib)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MDd -D_ITERATOR_DEBUG_LEVEL=2")
	set(COPY_TO ${CMAKE_CURRENT_SOURCE_DIR}/powrprof/debug/POWRPROF.lib)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MD -D_ITERATOR_DEBUG_LEVEL=0")
endif()


target_compile_features(demo PRIVATE cxx_std_17)


target_link_libraries(
	demo
	${POWRPROF_PATH}
)