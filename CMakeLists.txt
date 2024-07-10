cmake_minimum_required(VERSION 3.20.0 FATAL_ERROR)

project(canvas VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MD -D_ITERATOR_DEBUG_LEVEL=0")

# Uncomment if you want to see compile commands
# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Source files
file(GLOB CPP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(
    canvas
    SHARED
    ${CPP_SRC}
    include/imgui/imgui.cpp
    include/imgui/imgui_demo.cpp
    include/imgui/imgui_draw.cpp
    include/imgui/imgui_impl_vulkan.cpp
    include/imgui/imgui_impl_win32.cpp
    include/imgui/imgui_tables.cpp
    include/imgui/imgui_widgets.cpp
    # include/cpr/accept_encoding.cpp       #only include when you need
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

# Paths to libraries
# set(CURL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/release/libcurl.lib)   # Uncomment when needed
set(LIBMEM_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/release/libmem.lib)
set(VULKAN_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/vulkan.lib)
set(MINHOOK_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/release/minhook.lib)
set(COPY_TO ${CMAKE_CURRENT_SOURCE_DIR}/That_Sky_Mod/lib/release/powrprof.lib)

# Include directories
include_directories(
    include
    include/imgui
    include/libmem
    include/vulkan
    include/minhook/include
)

# Link libraries
target_link_libraries(
    canvas
    ntdll
    # ${CURL_PATH}
    ${LIBMEM_PATH}
    ${VULKAN_PATH}
    ${MINHOOK_PATH}
)

# Add source files for unresolved external symbols
file(GLOB ADDITIONAL_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hooks.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils.cpp
)

target_sources(canvas PRIVATE ${ADDITIONAL_SRC})
