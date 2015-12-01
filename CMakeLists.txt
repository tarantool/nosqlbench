cmake_minimum_required(VERSION 2.6)

project(nb)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

## build options
#
find_library(M_LIB m)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: Debug Release" FORCE)
endif()
##
## cflags
##
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -std=gnu99 -Wall -Wextra -g")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -march=native")
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb3")
endif()

include_directories("${PROJECT_SOURCE_DIR}/third_party")
include_directories("${PROJECT_SOURCE_DIR}/third_party/tarantool-c/include")
include_directories("${PROJECT_SOURCE_DIR}/third_party/memcached")
include_directories("${PROJECT_SOURCE_DIR}/third_party/memcached/memcached")

add_subdirectory(third_party)
add_subdirectory(sources)
add_subdirectory(plotter)

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
