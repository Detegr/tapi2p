cmake_minimum_required(VERSION 2.8.5)

project(tapi2p)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/dist/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/dist/lib)
set(WEB_OUTPUT_PATH ${PROJECT_BINARY_DIR}/dist/web)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
set(CMAKE_C_FLAGS "-pipe -g3 -std=c99 -D_GNU_SOURCE -fstack-protector -Wl,-z,relro -Wl,-z,now -Wformat-security -Wpointer-arith -Wformat -Wformat-nonliteral -Winit-self")
add_subdirectory(core)
add_subdirectory(test)

include_directories($OpenSSL_INCLUDE_DIRS)
include_directories(${PROJECT_SOURCE_DIR}/core)
include_directories(${PROJECT_SOURCE_DIR}/crypt)

add_executable(tapi2p_core core/main.c)
target_link_libraries(tapi2p_core ssl crypto pthread jansson tapi2p_corelib)

add_subdirectory(ui)
