
set(nb_bin nb)

set(nb_libs tntnet tnt m)
set(nb_src
	nb_arg.c
	nb_stat.c
	nb_func.c
	nb_test.c
	nb_opt.c
	nb_plot.c
	nb_data.c
	nb_cb.c
#	nb_redis.c
	nb_raw.c
	nb_thread.c
	nb.c
	nb_main.c)

add_executable(${nb_bin} ${nb_src})
target_link_libraries(${nb_bin} ${nb_libs})

install_targets(/bin ${nb_bin})
