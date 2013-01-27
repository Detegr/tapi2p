cmake_minimum_required(VERSION 2.8)

project(core)

add_definitions(-g3 -std=c99 -D_GNU_SOURCE)
find_package(OpenSSL REQUIRED)

include_directories($OpenSSL_INCLUDE_DIRS)
include_directories(${PROJECT_SOURCE_DIR}/core)
include_directories(${PROJECT_SOURCE_DIR}/crypt)
add_executable(tapi2p_core \\
			   core/main.c \\
			   core/core.c \\
			   core/config.c \\
			   core/pathmanager.c \\
			   core/peer.c \\
			   core/event.c \\
			   core/peermanager.c \\
			   core/pipemanager.c \\
			   core/ptrlist.c \\
			   crypt/aes.c \\
			   crypt/key.c \\
			   crypt/privatekey.c \\
			   crypt/publickey.c \\
			   crypt/keygen.c \\
			   )
target_link_libraries(tapi2p_core ssl crypto pthread)
add_subdirectory(test)
add_subdirectory(ui)
