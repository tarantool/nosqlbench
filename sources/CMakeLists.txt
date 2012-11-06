
set(nb_bin nb)

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

find_package (LevelDB QUIET)
if (LEVELDB_FOUND)
	set(nb_src ${nb_src}
		nb_db_leveldb.c
		nb_db_leveldb.h
	)

	include_directories(${LEVELDB_INCLUDE_DIRS})
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_LEVELDB")
endif (LEVELDB_FOUND)

add_executable(${nb_bin} ${nb_src})
target_link_libraries(${nb_bin} ${nb_libs})

if (LEVELDB_FOUND)
	target_link_libraries (${nb_bin} ${LEVELDB_LIBRARIES})
endif (LEVELDB_FOUND)
