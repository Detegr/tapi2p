cmake_minimum_required(VERSION 2.8)

project(AEStest)

add_definitions(-g3)
add_executable(aestest AES.cpp Key.cpp PrivateKey.cpp PublicKey.cpp)
find_package(OpenSSL REQUIRED)

include_directories($OpenSSL_INCLUDE_DIRS)
target_link_libraries(aestest ssl crypto)
