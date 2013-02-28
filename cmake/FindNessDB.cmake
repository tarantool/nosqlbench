# - Try to find libnessdb
# Once done this will define
#
#  NESSDB_FOUND - system has libnessdb
#  NESSDB_INCLUDE_DIR - the libnessdb include directory
#  NESSDB_LIBRARIES - libnessdb library
#

find_library(NESSDB_LIBRARIES
	NAMES nessdb
	PATHS "${CMAKE_SOURCE_DIR}/third_party"
	PATH_SUFFIXES nessDB
)

find_path(NESSDB_INCLUDE_DIRS
	NAMES engine/db.h
	PATHS "${CMAKE_SOURCE_DIR}/third_party"
	PATH_SUFFIXES nessDB
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NESSDB DEFAULT_MSG NESSDB_LIBRARIES NESSDB_INCLUDE_DIRS)
mark_as_advanced(NESSDB_INCLUDE_DIRS NESSDB_LIBRARIES)
