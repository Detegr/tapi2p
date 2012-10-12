cmake_minimum_required(VERSION 2.8)

project(AEStest)

add_definitions(-g3)
add_executable(tapi2p_chat chat.cpp AES.cpp Key.cpp PrivateKey.cpp PublicKey.cpp Config.cpp KeyGenerator.cpp PathManager.cpp PeerManager.cpp ui.cpp)
find_package(OpenSSL REQUIRED)
find_package(Curses REQUIRED)

include_directories($OpenSSL_INCLUDE_DIRS)
find_library(DTGLIB_LIBRARY dtglib ${PROJECT_SOURCE_DIR}/dtglib)
target_link_libraries(tapi2p_chat ssl crypto ${DTGLIB_LIBRARY} pthread ncurses)
