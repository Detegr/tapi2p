cmake_minimum_required(VERSION 2.8)

project(tapi2p)

set(CMAKE_C_FLAGS "-g3 -std=c99 -D_GNU_SOURCE")
add_subdirectory(core)
add_subdirectory(test)

find_package(OpenSSL REQUIRED)

include_directories($OpenSSL_INCLUDE_DIRS)
include_directories(${PROJECT_SOURCE_DIR}/core)
include_directories(${PROJECT_SOURCE_DIR}/crypt)

add_executable(tapi2p_core core/main.c)
target_link_libraries(tapi2p_core ssl crypto pthread tapi2p_corelib)

add_subdirectory(ui)
