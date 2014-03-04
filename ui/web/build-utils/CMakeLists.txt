cmake_minimum_required(VERSION 2.8.5)
project(tapi2p-web-ui)

message(STATUS ${CMAKE_SOURCE_DIR}/core/event.h)
message(STATUS ${CMAKE_SOURCE_DIR}/ui/web/js/tapi2p-backend.js)
message(STATUS ${WEB_OUTPUT_PATH}/js/tapi2p-backend.js)
message(STATUS ${PROJECT_SOURCE_DIR})

add_custom_target(tapi2p-web-ui ALL
	COMMAND
	sh
	tapi2p-js-backend-replace-enum.sh
	${CMAKE_SOURCE_DIR}/core/event.h
	${CMAKE_SOURCE_DIR}/ui/web/js/tapi2p-backend.js
	${WEB_OUTPUT_PATH}/js/tapi2p-backend.js
	DEPENDS tapi2pws
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
