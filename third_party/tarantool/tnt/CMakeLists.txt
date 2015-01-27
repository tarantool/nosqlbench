#============================================================================#
# build flags
#============================================================================#

#============================================================================#
# Build tnt project
#============================================================================#

#
# source files
#
set (tnt_sources
	tnt_mem.c
	tnt_enc.c
	tnt_tuple.c
	tnt_iter.c
	tnt_stream.c
	tnt_buf.c
	tnt_ping.c
	tnt_insert.c
	tnt_update.c
	tnt_delete.c
	tnt_call.c
	tnt_select.c
	tnt_reply.c
	tnt_request.c)

#----------------------------------------------------------------------------#
# Builds
#----------------------------------------------------------------------------#

#
# Static library
#
add_library(tnt STATIC ${tnt_sources})
set_target_properties(tnt PROPERTIES COMPILE_FLAGS "${tnt_cflags}")
set_target_properties(tnt PROPERTIES VERSION ${LIBTNT_VERSION} SOVERSION ${LIBTNT_SOVERSION})

