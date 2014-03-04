cmake_minimum_required(VERSION 2.8.5)
project(tapi2p-web-ui)

add_subdirectory(build-utils)
add_subdirectory(server)

file(COPY
	${PROJECT_SOURCE_DIR}/index.html
	${PROJECT_SOURCE_DIR}/css
	${PROJECT_SOURCE_DIR}/js
	${PROJECT_SOURCE_DIR}/img
	DESTINATION ${WEB_OUTPUT_PATH})
