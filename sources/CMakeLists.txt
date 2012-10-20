
set(nb_bin nb2)

set(nb_libs "${TARANTOOL_LIB}" "${TARANTOOLNET_LIB}" "${TARANTOOLSQL_LIB}" ${M_LIB})
set(nb_src
	nb_alloc.h
	nb.c
	nb_config.c
	nb_config.h
	nb_db.c
	nb_db.h
	nb_db_tarantool.c
	nb_db_tarantool.h
	nb_engine.c
	nb_engine.h
	nb.h
	nb_key.c
	nb_key.h
	nb_opt.c
	nb_opt.h
	nb_report.c
	nb_report.h
	nb_stat.c
	nb_stat.h
	nb_warmup.c
	nb_warmup.h
	nb_worker.c
	nb_worker.h
	nb_workload.c
	nb_workload.h
)

add_executable(${nb_bin} ${nb_src})
target_link_libraries(${nb_bin} ${nb_libs})

install_targets(/bin ${nb_bin})
install_files(/etc/tarantool nosqlbench.conf)
