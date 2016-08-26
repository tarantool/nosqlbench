# - Try to find libev
# Once done this will define
#
#  LIBEV_FOUND - system has libnessdb
#  LIBEV_INCLUDE_DIR - the libnessdb include directory
#  LIBEV_LIBRARIES - libnessdb library
#

find_path(LIBEV_INCLUDE_DIR NAMES ev.h)
find_library(LIBEV_LIBRARIES NAMES ev)

if(LIBEV_INCLUDE_DIR AND LIBEV_LIBRARIES)
    set(LIBEV_FOUND ON)
endif(LIBEV_INCLUDE_DIR AND LIBEV_LIBRARIES)

if(LIBEV_FOUND)
    if (NOT LIBEV_FIND_QUIETLY)
        message(STATUS "Found libev includes: ${LIBEV_INCLUDE_DIR}/ev.h")
        message(STATUS "Found libev library: ${LIBEV_LIBRARIES}")
    endif (NOT LIBEV_FIND_QUIETLY)
else(LIBEV_FOUND)
    if (LIBEV_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find libev development files")
    endif (LIBEV_FIND_REQUIRED)
endif (LIBEV_FOUND)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBEV DEFAULT_MSG LIBEV_LIBRARIES LIBEV_INCLUDE_DIR)
mark_as_advanced(LIBEV_INCLUDE_DIR LIBEV_LIBRARIES)
