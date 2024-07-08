cmake_minimum_required(VERSION 3.20.0 FATAL_ERROR)

project(canvas VERSION 0.1.0 LANGUAGES CXX C)

# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)     #uncomment if you wanna see compile commands

file(GLOB CPP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp )

add_library(
	powrprof
	SHARED
	${CPP_SRC}
	include/imgui/imgui.cpp
	include/imgui/imgui_demo.cpp
	include/imgui/imgui_draw.cpp
	include/imgui/imgui_impl_vulkan.cpp
	include/imgui/imgui_impl_win32.cpp
	include/imgui/imgui_tables.cpp
	include/imgui/imgui_widgets.cpp
    # include/cpr/accept_encoding.cpp		#only include when you need
    # include/cpr/async.cpp
    # include/cpr/auth.cpp
    # include/cpr/bearer.cpp
    # include/cpr/callback.cpp
    # include/cpr/cert_info.cpp
    # include/cpr/cookies.cpp
    # include/cpr/cprtypes.cpp
    # include/cpr/curl_container.cpp
    # include/cpr/curlholder.cpp
    # include/cpr/error.cpp
    # include/cpr/file.cpp
    # include/cpr/multipart.cpp
    # include/cpr/parameters.cpp
    # include/cpr/payload.cpp
    # include/cpr/proxies.cpp
    # include/cpr/proxyauth.cpp
    # include/cpr/session.cpp
    # include/cpr/threadpool.cpp
    # include/cpr/timeout.cpp
    # include/cpr/unix_socket.cpp
    # include/cpr/util.cpp
    # include/cpr/response.cpp
    # include/cpr/redirect.cpp
    # include/cpr/interceptor.cpp
    # include/cpr/ssl_ctx.cpp
    # include/cpr/curlmultiholder.cpp
    # include/cpr/multiperform.cpp
	)



# set(CURL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/release/libcurl.lib)   #uncomment When Needed
set(LIBMEM_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/release/libmem.lib)
set(VULKAN_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/vulkan.lib)
# set(COPY_TO ${CMAKE_CURRENT_SOURCE_DIR}/mod-demo/powrprof/release/powrprof.lib)   #mod demo is not used rn
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
    # ${CURL_PATH}
    ${LIBMEM_PATH}
	${VULKAN_PATH}
)


add_custom_command(
	TARGET powrprof POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_LINKER_FILE:powrprof> ${COPY_TO}
)

# add_custom_target(BeforeBuild DEPENDS ${COPY_TO})

# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/mod-demo)