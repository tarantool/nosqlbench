#
# source files
#

set (tntsql_sources
	tnt_utf8.c
	tnt_lex.c
	tnt_sql.c)

#----------------------------------------------------------------------------#
# Builds
#----------------------------------------------------------------------------#

add_library(tntsql STATIC ${tntsql_sources})
set_target_properties(tntsql PROPERTIES COMPILE_FLAGS "${tnt_cflags}")
set_target_properties(tntsql PROPERTIES VERSION ${LIBTNT_VERSION} SOVERSION ${LIBTNT_SOVERSION})
