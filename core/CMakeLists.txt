cmake_minimum_required(VERSION 2.8)

project(core)

set(CMAKE_C_FLAGS "-g3 -std=c99 -D_GNU_SOURCE -fPIC")
find_package(OpenSSL REQUIRED)

add_library(tapi2p_corelib SHARED \\
			core.c \\
			pathmanager.c \\
			peer.c \\
			event.c \\
			peermanager.c \\
			pipemanager.c \\
			ptrlist.c \\
			handlers.c \\
			../crypto/aes.c \\
			../crypto/key.c \\
			../crypto/privatekey.c \\
			../crypto/publickey.c \\
			../crypto/keygen.c \\
			../dtgconf/src/config.c
)

