
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

extern struct nb nb;

extern volatile sig_atomic_t nb_signaled;

static void nb_warmup_progressbar(int processed) {
	float percent = processed * 100.0 / nb.opts.request_count;
	int cols = 40;
	int done = percent * cols / 100.0;
	int left = cols - done;
	printf("Warmup [");
	while (done-- >= 0)
		printf(".");
	while (left-- > 0)
		printf(" ");
	printf("] %.2f%%\r", percent);
	fflush(NULL);

}

int nb_warmup(void)
{
	int rc = 0;
	struct nb_db db = { nb.db, NULL };

	struct nb_key key;
	nb.key->init(&key, nb.key_dist);

	nb.db->init(&db, 1);
	if (nb.db->connect(&db, nb.opts.host, nb.opts.port) == -1) {
		rc = 1;
		goto free;
	}

	int missed = 0;
	uint32_t i = 0;
	while (i < nb.opts.request_count && !nb_signaled) {
		nb.key->generate_by_id(&key, i);
		nb.db->insert(&db, &key);
		i++;
		if (i % nb.opts.request_batch_count == 0 && i > 0) {
			nb.db->recv(&db, nb.opts.request_batch_count, &missed);
			nb_warmup_progressbar(i);
		}
	}
	if (!nb_signaled)
		nb.db->recv(&db, nb.opts.request_batch_count, &missed);
free:
	nb.key->free(&key);
	nb.db->close(&db);
	nb.db->free(&db);
	return rc;
}
