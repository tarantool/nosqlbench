#============================================================================#
# build flags
#============================================================================#

# default flags
if ("${CMAKE_BUILD_TYPE}" STREQUAL "None")
    set (tntnet_cflags "-std=gnu99")
else()
    set (tntnet_cflags "-std=gnu99 -Wall -Wextra")
    set (tntnet_cflags "${tntnet_cflags} -Wno-sign-compare -Wno-strict-aliasing")
endif()

# Only add -Werror if it's a debug build, done by developers.
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set (tntnet_cflags "${tntnet_cflags} -Werror")
endif()
set (tntnet_cflags "${tntnet_cflags} -D_XOPEN_SOURCE")

#============================================================================#
# Build tnt net project
#============================================================================#

#
# source files
#

set (tntnet_sources
	tnt_iob.c
	tnt_io.c
	tnt_opt.c
	tnt_net.c)

#----------------------------------------------------------------------------#
# Builds
#----------------------------------------------------------------------------#

add_library(tntnet STATIC ${tntnet_sources})
set_target_properties(tntnet PROPERTIES COMPILE_FLAGS "${tntnet_cflags}")
set_target_properties(tntnet PROPERTIES VERSION ${LIBTNT_VERSION} SOVERSION ${LIBTNT_SOVERSION})

