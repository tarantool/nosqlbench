
SET(nosqlb_bin  nosqlb)

SET(nosqlb_libs tnt)
SET(nosqlb_src
	nosqlb_arg.c
	nosqlb_stat.c
	nosqlb_func.c
	nosqlb_test.c
	nosqlb_opt.c
	nosqlb_plot.c
	nosqlb_cb.c
	nosqlb_redis.c
	nosqlb.c
	nosqlb_main.c)

ADD_EXECUTABLE(${nosqlb_bin} ${nosqlb_src})
TARGET_LINK_LIBRARIES(${nosqlb_bin} ${nosqlb_libs})

INSTALL_TARGETS(/bin ${nosqlb_bin})
