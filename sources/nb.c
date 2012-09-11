
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
	va_end(args);
	fflush(NULL);
	exit(1);
}

volatile sig_atomic_t nb_signaled = 0;

static void nb_sigcb(int sig)
{
	nb_signaled = 1;
}

static void nb_init(void)
{
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
	/* checking request distributions */
	if (nb.opts.request_count == 0)
		nb_error("bad request distribution");
	if (nb.opts.dist_replace +
	    nb.opts.dist_update +
	    nb.opts.dist_select +
	    nb.opts.dist_delete == 0)
		nb_error("bad request distribution");
	/* initialize statistics */
	nb_statistics_init(&nb.stats, nb.opts.threads_start);
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
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = nb_sigcb;
	if (sigaction(SIGINT, &sa, NULL) == -1)
		nb_error("signal initialization failed\n");
}

static void nb_free(void)
{
	nb_statistics_free(&nb.stats);
	nb_workers_free(&nb.workers);
	if (nb.report->free)
		nb.report->free();
}

int main(int argc, char * argv[])
{
	memset(&nb, 0, sizeof(struct nb));

	nb_opt_init(&nb.opts);
	/* read configuration */

	nb_init();

	nb.report->report_start();

	int rc = nb_warmup();
	if (rc) {
		nb_free();
		return rc;
	}

	nb_engine();

	nb_free();
	return rc;
}
