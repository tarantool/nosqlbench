#============================================================================#
# build flags
#============================================================================#

set (tntnet_cflags "${tnt_cflags} -D_XOPEN_SOURCE")

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

