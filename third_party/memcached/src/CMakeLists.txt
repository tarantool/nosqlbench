set(LIBMC_NAME "mc")
set(LIBMC_VERSIONMAJOR "1")
set(LIBMC_VERSIONMINOR "0")
set(LIBMC_VERSION "${LIBMC_VERSIONMAJOR}.${LIBMC_VERSIONMINOR}")
set(LIBMC_SOVERSION "${LIBMC_VERSIONMAJOR}")
set(LIBMC_CFLAGS "-std=gnu99 -Wall -Wextra -g")
message(STATUS "C_FLAGS: ${LIBMC_CFLAGS}")

set(libtb_sources
    mc.c)

add_library(mc STATIC ${libtb_sources})

set_target_properties(mc PROPERTIES COMPILE_FLAGS "${LIBMC_CFLAGS}")
set_target_properties(mc PROPERTIES VERSION ${LIBMC_VERSION} SOVERSION ${LIBMC_SOVERSION})
set_target_properties(mc PROPERTIES OUTPUT_NAME ${LIBMC_NAME})

