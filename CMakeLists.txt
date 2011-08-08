CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
INCLUDE(CheckLibraryExists)
INCLUDE(CheckIncludeFiles)

PROJECT(nosqlb)

## build options
##
option(LIBTNT_INCLUDE
	"libtnt include path" "/usr/include")
option(LIBTNT_LIB
	"libtnt lib path" "/usr/lib")

## libtnt matching
##
FIND_PATH(LIBTNT_HEADER_DIR tnt.h "${LIBTNT_INCLUDE}")
IF (NOT LIBTNT_HEADER_DIR)
	MESSAGE(FATAL_ERROR 
	"Tarantool DB client library header not found")
ENDIF()

FIND_PATH(LIBTNT_LIB_DIR libtnt.a "${LIBTNT_LIB}")
IF (NOT LIBTNT_LIB_DIR)
	MESSAGE(FATAL_ERROR 
	"Tarantool DB client library (-ltnt) not found")
ENDIF()

## cflags
##
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -std=gnu99 -pedantic -Wall -Wextra")

INCLUDE_DIRECTORIES("${PROJECT_SOURCE_DIR}/src")
INCLUDE_DIRECTORIES("${LIBTNT_HEADER_DIR}")

LINK_DIRECTORIES("${LIBTNT_LIB_DIR}")

ADD_SUBDIRECTORY(src)

INSTALL(FILES README LICENSE DESTINATION doc)
