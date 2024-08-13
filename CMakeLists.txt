cmake_minimum_required(VERSION 3.20.0 FATAL_ERROR)

project(canvas VERSION 0.1.0 LANGUAGES CXX C)

# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)     #uncomment if you wanna see compile commands

file(GLOB CPP_SRC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp 
)

add_library(
	powrprof
	SHARED
	${CPP_SRC}
)


set(LIBS 
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/release/libcpr.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/release/libcurl_a.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/release/libmem.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/release/libimgui.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/release/vulkan.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/release/ktx.lib
)
# set(COPY_TO ${CMAKE_CURRENT_SOURCE_DIR}/mod-demo/powrprof/release/powrprof.lib)   #mod demo is not used rn uncomment when needed
set(COPY_TO ${CMAKE_CURRENT_SOURCE_DIR}/That_Sky_Mod/lib/release/powrprof.lib)      

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MD -D_ITERATOR_DEBUG_LEVEL=0")


include_directories(
	include
	include/imgui
	include/libmem
)

target_compile_features(powrprof PRIVATE cxx_std_20)

target_link_libraries(
	powrprof
	ntdll
	Ws2_32
    crypt32
    Normaliz
    Wldap32
    ${LIBS}
)


add_custom_command(
	TARGET powrprof POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_LINKER_FILE:powrprof> ${COPY_TO}
)

# add_custom_target(BeforeBuild DEPENDS ${COPY_TO})

# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/mod-demo)