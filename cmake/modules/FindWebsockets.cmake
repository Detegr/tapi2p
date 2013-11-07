# This module tries to find WEBSOCKETS library and include files
#
# WEBSOCKETS_INCLUDE_DIR, path where to find WEBSOCKETS.h
# WEBSOCKETS_LIBRARY, the library to link against
# WEBSOCKETS_FOUND, If false, do not try to use WEBSOCKETS
#
# This currently works probably only for Linux

SET(LIBRARY_PATHS
	/usr/lib
	/usr/local/lib)

FIND_PATH(WEBSOCKETS_INCLUDE_DIR libwebsockets.h
    /usr/local/include
    /usr/include)

FIND_LIBRARY(WEBSOCKETS_LIBRARY
	NAMES libwebsockets.so
	PATHS ${LIBRARY_PATHS})

IF(WEBSOCKETS_INCLUDE_DIR AND WEBSOCKETS_LIBRARY)
	SET(WEBSOCKETS_FOUND TRUE)
	SET(WEBSOCKETS_INCLUDE_DIR ${WEBSOCKETS_INCLUDE_DIR} CACHE STRING "The include directory  needed to use libwebsockets")
	SET(WEBSOCKETS_LIBRARY ${WEBSOCKETS_LIBRARY} CACHE STRING "The libraries needed to use libwebsockets")
ENDIF( WEBSOCKETS_INCLUDE_DIR AND WEBSOCKETS_LIBRARY )

IF(WEBSOCKETS_FOUND)
	IF(NOT Websockets_FIND_QUIETLY)
		message(STATUS "Found libwebsockets: ${WEBSOCKETS_LIBRARY}")
	ENDIF(NOT Websockets_FIND_QUIETLY)
ELSE(WEBSOCKETS_FOUND)
	IF(Websockets_FIND_REQUIRED)
		message(FATAL_ERROR "Could not find libwebsockets library")
	ENDIF(Websockets_FIND_REQUIRED)
ENDIF(WEBSOCKETS_FOUND)

MARK_AS_ADVANCED(
    WEBSOCKETS_INCLUDE_DIR
    WEBSOCKETS_LIBRARY
)
