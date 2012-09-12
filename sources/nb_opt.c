
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "nb_alloc.h"
#include "nb_opt.h"

void nb_opt_init(struct nb_options *opts)
{
	opts->benchmark_policy = NB_BENCHMARK_NOLIMIT;
	opts->report_interval = 1;
	opts->time_limit = 0;
	opts->report = nb_strdup("default");
	opts->csv_file = NULL;
	opts->threads_policy = NB_THREADS_ATONCE;
	opts->threads_start = 10;
	opts->threads_max = 10;
	opts->threads_interval = 1;
	opts->threads_increment = 1;
	opts->request_count = 10000;
	opts->request_batch_count = 1000;
	opts->history_per_batch = 16;
	opts->db = nb_strdup("tarantool");
	opts->key = nb_strdup("string");
	opts->key_dist = nb_strdup("uniform");
	opts->key_dist_iter = 4;
	opts->value_size = 16;
	opts->dist_replace = 40;
	opts->dist_update = 10;
	opts->dist_delete = 10;
	opts->dist_select = 40;
	opts->host = nb_strdup("127.0.0.1");
	opts->port = 33013;
}

void nb_opt_free(struct nb_options *opts)
{
	free(opts->report);
	free(opts->csv_file);
	free(opts->db);
	free(opts->key);
	free(opts->key_dist);
	free(opts->host);
}
