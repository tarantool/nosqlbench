cmake_minimum_required(VERSION 2.6)

project(nosqlb)

include(CheckLibraryExists)
include(CheckIncludeFiles)

## build options
##
option(LIBTNT_INCLUDE
	"libtnt include path" "/usr/include")
option(LIBTNT_LIB
	"libtnt lib path" "/usr/lib")
option(LIBTNTNET_INCLUDE
	"libtntnet include path" "/usr/include")
option(LIBTNTNET_LIB
	"libtntnet lib path" "/usr/lib")

## libtnt matching
##
find_path(LIBTNT_HEADER_DIR libtnt/tnt.h "${LIBTNT_INCLUDE}")
if (NOT LIBTNT_HEADER_DIR)
	message(FATAL_ERROR 
	"Tarantool DB client library header not found")
endif()
set(LIBTNT_HEADER_DIR "${LIBTNT_INCLUDE}/libtnt")

find_path(LIBTNT_LIB_DIR libtnt.a "${LIBTNT_LIB}")
if (NOT LIBTNT_LIB_DIR)
	message(FATAL_ERROR 
	"Tarantool DB client library (-ltnt) not found")
endif()

find_path(LIBTNTNET_HEADER_DIR libtnt/tnt_net.h "${LIBTNTNET_INCLUDE}")
if (NOT LIBTNTNET_HEADER_DIR)
	message(FATAL_ERROR 
	"Tarantool DB client library header not found")
endif()
set(LIBTNTNET_HEADER_DIR "${LIBTNT_INCLUDE}/libtnt")

find_path(LIBTNTNET_LIB_DIR libtntnet.a "${LIBTNTNET_LIB}")
if (NOT LIBTNTNET_LIB_DIR)
	message(FATAL_ERROR 
	"Tarantool DB client library (-ltntnet) not found")
endif()

## cflags
##
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -O3 -g -std=gnu99 -pedantic -Wall -Wextra")

include_directories("${PROJECT_SOURCE_DIR}/src")
include_directories("${LIBTNT_HEADER_DIR}")
include_directories("${LIBTNTNET_HEADER_DIR}")

link_directories("${LIBTNT_LIB_DIR}")
link_directories("${LIBTNTNET_LIB_DIR}")

add_subdirectory(src)

install(FILES README LICENSE DESTINATION doc)
