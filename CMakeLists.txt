cmake_minimum_required(VERSION 2.6)

project(nb)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

## build options
#
find_library(M_LIB m)

## cflags
##
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -std=gnu99 -Wall -Wextra -g")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g -pg")
set(CMAKE_C_FLAGS_RELEASE "-O3 -march=native")

include_directories("${PROJECT_SOURCE_DIR}/sources")
include_directories("${PROJECT_SOURCE_DIR}/third_party/msgpuck")
include_directories("${PROJECT_SOURCE_DIR}/third_party/tarantool-c")
include_directories("${PROJECT_SOURCE_DIR}/third_party")

add_definitions("-DTB_LOCAL=${PROJECT_SOURCE_DIR}/third_party/tarantool-c/lib")

add_subdirectory(third_party)
add_subdirectory(sources)
add_subdirectory(plotter)
