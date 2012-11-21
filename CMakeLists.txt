cmake_minimum_required(VERSION 2.8)

project(AEStest)

add_definitions(-g3)
find_package(OpenSSL REQUIRED)
find_package(Curses REQUIRED)

include_directories($OpenSSL_INCLUDE_DIRS)
include_directories(${PROJECT_SOURCE_DIR}/dtglib)
add_library(dtglib STATIC dtglib/Concurrency.cpp dtglib/Network.cpp dtglib/Packet.cpp)
add_executable(tapi2p_core core.cpp AES.cpp Key.cpp PrivateKey.cpp PublicKey.cpp Config.cpp KeyGenerator.cpp PathManager.cpp PeerManager.cpp)
target_link_libraries(tapi2p_core ssl crypto pthread dtglib)
