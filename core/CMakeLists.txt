cmake_minimum_required(VERSION 2.8)

project(core)

set(CMAKE_C_FLAGS "-g3 -std=c99 -D_GNU_SOURCE -fPIC")
find_package(OpenSSL REQUIRED)
find_package(Jansson REQUIRED)

add_library(tapi2p_corelib SHARED \\
			core.c \\
			pathmanager.c \\
			peer.c \\
			event.c \\
			peermanager.c \\
			pipemanager.c \\
			ptrlist.c \\
			handlers.c \\
			util.c \\
			file.c \\
			../crypto/aes.c \\
			../crypto/key.c \\
			../crypto/privatekey.c \\
			../crypto/publickey.c \\
			../crypto/keygen.c \\
			../dtgconf/src/config.c
)

target_link_libraries(tapi2p_corelib jansson)
