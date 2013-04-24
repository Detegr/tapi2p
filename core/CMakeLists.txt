cmake_minimum_required(VERSION 2.8)

project(core)

add_definitions(-g3 -std=c99 -D_GNU_SOURCE -fPIC)
find_package(OpenSSL REQUIRED)

add_library(tapi2p_corelib SHARED \\
			../core/core.c \\
			../core/config.c \\
			../core/pathmanager.c \\
			../core/peer.c \\
			../core/event.c \\
			../core/peermanager.c \\
			../core/pipemanager.c \\
			../core/ptrlist.c \\
			../crypt/aes.c \\
			../crypt/key.c \\
			../crypt/privatekey.c \\
			../crypt/publickey.c \\
			../crypt/keygen.c \\
)

