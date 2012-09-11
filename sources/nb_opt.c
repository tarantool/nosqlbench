
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <tarantool/tnt.h>
#include <tarantool/tnt_queue.h>
#include <tarantool/tnt_utf8.h>
#include <tarantool/tnt_lex.h>

#include "nb_alloc.h"
#include "nb_opt.h"

void nb_opt_init(struct nb_options *opts)
{
	opts->benchmark_policy = NB_BENCHMARK_NOLIMIT;
	//opts->benchmark_policy = NB_BENCHMARK_TIMELIMIT;
	//opts->benchmark_policy = NB_BENCHMARK_THREADLIMIT;
	
	opts->report_interval = 1;
	opts->time_limit = 5;
	opts->report = "default";

	opts->csv_file = NULL;
	//opts->csv_file = "benchmark.csv";

	opts->threads_policy = NB_THREADS_ATONCE;
	opts->threads_start = 10;
	opts->threads_max = 20;
	opts->threads_interval = 1;
	opts->threads_increment = 1;

	opts->request_count = 10000;
	opts->request_batch_count = 1000;

	opts->history_per_batch = 16;

	opts->db = "tarantool";
	opts->key = "string";
	opts->key_dist = "uniform";
	//opts->key_dist = "gaussian";
	opts->key_dist_iter = 4;

	opts->value_size = 16;

	opts->dist_replace = 40;
	opts->dist_update = 10;
	opts->dist_delete = 10;
	opts->dist_select = 40;

	opts->host = "127.0.0.1";
	opts->port = 33013;
}

static struct tnt_lex_keyword tc_lex_keywords[] =
{
	{ NULL, 0, TNT_TK_NONE }
};

static int nb_opt_process(struct tnt_lex *lex) {
	struct tnt_tk *tk;
	switch (tnt_lex(lex, &tk)) {
	}
	return 0;
}

int nb_opt_configure(struct nb_options *opts, char *file)
{
	struct stat st;
	if (stat(file, &st) == -1) {
		printf("error: stat(): %s\n", strerror(errno));
		return -1;
	}
	int fd = open(file, O_RDONLY);
	if (fd == -1)
		return -1;
	char *buf = nb_malloc(st.st_size);
	size_t off = 0;
	do {
		ssize_t r = read(fd, buf + off, st.st_size - off);
		if (r == -1) {
			close(fd);
			free(buf);
			printf("error: read(): %s\n", strerror(errno));
			return -1;
		}
		off += r;
	} while (off != st.st_size);
	close(fd);

	struct tnt_lex lex;
	if (tnt_lex_init(&lex, tc_lex_keywords, (unsigned char*)buf, st.st_size) == -1) {
		free(buf);
		return -1;
	}
	free(buf);

	int rc = nb_opt_process(&lex);

	tnt_lex_free(&lex);
	return rc;
}
