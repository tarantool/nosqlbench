
/*
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include "nosqlbench.h"

struct nb nb;

static void nb_error(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	printf("error: ");
	vprintf(fmt, args);
	printf("\n");
	va_end(args);
	fflush(NULL);
	exit(1);
}

volatile sig_atomic_t nb_signaled = 0;

static void nb_sigcb(int sig) {
	(void)sig;
	nb_signaled = 1;
}

static void nb_init_signal(void)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = nb_sigcb;
	if (sigaction(SIGINT, &sa, NULL) == -1)
		nb_error("signal initialization failed\n");
}

static void nb_validate(void)
{
	/* validating benchmark_policy */
	if (nb.opts.benchmark_policy_name) {
		if (!strcmp(nb.opts.benchmark_policy_name, "time_limit"))
			nb.opts.benchmark_policy = NB_BENCHMARK_TIMELIMIT;
		else
		if (!strcmp(nb.opts.benchmark_policy_name, "thread_limit"))
			nb.opts.benchmark_policy = NB_BENCHMARK_THREADLIMIT;
		else
		if (!strcmp(nb.opts.benchmark_policy_name, "no_limit"))
			nb.opts.benchmark_policy = NB_BENCHMARK_NOLIMIT;
		else
			nb_error("bad benchmarking policy '%s'",
				 nb.opts.benchmark_policy_name);
	}
	/* validating client_creation_policy */
	if (nb.opts.threads_policy_name) {
		if (!strcmp(nb.opts.threads_policy_name, "at_once"))
			nb.opts.threads_policy = NB_THREADS_ATONCE;
		else
		if (!strcmp(nb.opts.threads_policy_name, "interval"))
			nb.opts.threads_policy = NB_THREADS_INTERVAL;
		else
			nb_error("bad client creation policy '%s'",
				 nb.opts.threads_policy_name);
	}
	/* matching and validation specified interfaces */
	nb.db = nb_db_match(nb.opts.db);
	if (nb.db == NULL)
		nb_error("db interface '%s' not found", nb.opts.db);
	nb.key = nb_key_match(nb.opts.key);
	if (nb.key == NULL)
		nb_error("key interface '%s' not found", nb.opts.key);
	nb.key_dist = nb_key_distribution_match(nb.opts.key_dist);
	if (nb.key == NULL)
		nb_error("key distribution interface '%s' not found", nb.opts.key_dist);
	nb.report = nb_report_match(nb.opts.report);
	if (nb.report == NULL)
		nb_error("report interface '%s' not found", nb.opts.report);
	/* validating request distributions */
	if (nb.opts.request_count == 0)
		nb_error("bad request distribution");
	int dist = nb.opts.dist_replace + nb.opts.dist_update +
		   nb.opts.dist_select +
		   nb.opts.dist_delete;
	if (dist <= 0)
		nb_error("bad request distribution");
	if (dist < 100)
		nb_error("request distribution is lower than 100%");
	if (dist > 100)
		nb_error("request distribution is higher than 100%");
	/* validating threads distributions */
	if (nb.opts.threads_policy == NB_THREADS_ATONCE &&
	    nb.opts.threads_max <= 0)
		nb_error("bad threads_max count");
	if (nb.opts.threads_policy == NB_THREADS_INTERVAL &&
	    nb.opts.threads_start <= 0)
		nb_error("bad threads_start count");
}

static void nb_init(void)
{
	/* validating current configuration options */
	nb_validate();
	/* initialize statistics */
	int statmax = (nb.opts.threads_policy == NB_THREADS_ATONCE) ?
		       nb.opts.threads_max :
		       nb.opts.threads_start;
	nb_statistics_init(&nb.stats, statmax);
	/* initialize workload */
	nb_workers_init(&nb.workers);
	nb_workload_init(&nb.workload, nb.opts.request_count);
	nb_workload_add(&nb.workload, NB_REPLACE, nb.db->replace, nb.opts.dist_replace);
	nb_workload_add(&nb.workload, NB_UPDATE, nb.db->update, nb.opts.dist_update);
	nb_workload_add(&nb.workload, NB_SELECT, nb.db->select, nb.opts.dist_select);
	nb_workload_add(&nb.workload, NB_DELETE, nb.db->del, nb.opts.dist_delete);
	nb_workload_link(&nb.workload);
	/* initialize key distribution */
	if (nb.key_dist->init)
		nb.key_dist->init(nb.opts.key_dist_iter);
	/* initialize report interface */
	if (nb.report->init)
		nb.report->init();
	/* initialize signal handler */
	nb_init_signal();
}

static void nb_free(void)
{
	nb_statistics_free(&nb.stats);
	nb_workers_free(&nb.workers);
	if (nb.report && nb.report->free)
		nb.report->free();
	nb_opt_free(&nb.opts);
}

static int nb_usage(char *binary)
{
	printf("NoSQL Benchmarking.\n\n");
	printf("usage: %s [config_file_path]\n", binary);
	return 1;
}

int main(int argc, char *argv[])
{
	int rc = 0;
	memset(&nb, 0, sizeof(struct nb));

	char *config = NB_DEFAULT_CONFIG;
	if (argc == 2) {
		if (!strcmp(argv[1], "-h") ||
		    !strcmp(argv[1], "--help"))
			return nb_usage(argv[0]);
		config = argv[1];
	}
	if (argc > 1 && argc != 2)
		return nb_usage(argv[0]);

	nb_opt_init(&nb.opts);

	if (nb_config_parse(config) == -1) {
		rc = 1;
		goto done;
	}

	nb_init();
	nb.report->report_start();

	rc = nb_warmup();
	if (rc || nb_signaled)
		goto done;

	nb_engine();
done:
	nb_free();
	return rc;
}
