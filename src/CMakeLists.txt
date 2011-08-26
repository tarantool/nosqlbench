
set(nosqlb_bin  nosqlb)

set(nosqlb_libs tnt)
set(nosqlb_src
	nosqlb_arg.c
	nosqlb_stat.c
	nosqlb_func.c
	nosqlb_test.c
	nosqlb_opt.c
	nosqlb_plot.c
	nosqlb_cb.c
	nosqlb_redis.c
	nosqlb_thread.c
	nosqlb.c
	nosqlb_main.c)

add_executable(${nosqlb_bin} ${nosqlb_src})
target_link_libraries(${nosqlb_bin} ${nosqlb_libs})

install_targets(/bin ${nosqlb_bin})
