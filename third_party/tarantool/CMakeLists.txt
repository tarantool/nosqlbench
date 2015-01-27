
#
# library soname version
#
set(LIBTNT_VERSION_MAJOR "1")
set(LIBTNT_VERSION_MINOR "1")

set(LIBTNT_VERSION   "${LIBTNT_VERSION_MAJOR}.${LIBTNT_VERSION_MINOR}")
set(LIBTNT_SOVERSION "${LIBTNT_VERSION_MAJOR}")

# default flags
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set (tnt_cflags "-std=gnu99")
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set (tnt_cflags "-std=gnu99 -Wall -Wextra -Werror")
    set (tnt_cflags "${tnt_cflags} -Wno-sign-compare -Wno-strict-aliasing")
endif()

add_subdirectory(tnt)
add_subdirectory(tntnet)
add_subdirectory(tntsql)
