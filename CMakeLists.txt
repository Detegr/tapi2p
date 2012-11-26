cmake_minimum_required(VERSION 2.8)

project(AEStest)

add_definitions(-g3)
find_package(OpenSSL REQUIRED)
find_package(Curses REQUIRED)

include_directories($OpenSSL_INCLUDE_DIRS)
include_directories(${PROJECT_SOURCE_DIR}/dtglib)
include_directories(${PROJECT_SOURCE_DIR}/core)
include_directories(${PROJECT_SOURCE_DIR}/crypt)
add_library(dtglib STATIC dtglib/Concurrency.cpp dtglib/Network.cpp dtglib/Packet.cpp)
add_executable(tapi2p_core \\
			   core/main.cpp \\
			   core/core.cpp \\
			   core/config.cpp \\
			   core/pathmanager.cpp \\
			   core/peermanager.cpp \\
			   core/pipemanager.cpp \\
			   crypt/aes.cpp \\
			   crypt/key.cpp \\
			   crypt/privatekey.cpp \\
			   crypt/publickey.cpp \\
			   crypt/keygenerator.cpp)
target_link_libraries(tapi2p_core ssl crypto pthread dtglib)
