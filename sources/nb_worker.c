
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>

#include "nb_alloc.h"
#include "nb_key.h"
#include "nb_db.h"
#include "nb_workload.h"
#include "nb_stat.h"
#include "nb_worker.h"

void nb_workers_init(struct nb_workers *workers)
{
	workers->head = NULL;
	workers->tail = NULL;
	workers->count = 0;
}

void nb_workers_free(struct nb_workers *workers)
{
	struct nb_worker *c = workers->head, *n;
	while (c) {
		n = c->next;
		c->db.dif->close(&c->db);
		c->db.dif->free(&c->db);
		c->key->free(&c->keyv);
		nb_history_free(&c->history);
		free(c);
		c = n;
	}
}

struct nb_worker*
nb_workers_create(struct nb_workers *workers, struct nb_db_if *dif,
		  struct nb_key_if *kif,
		  struct nb_key_distribution_if *distif,
		  struct nb_workload *workload, int history_max,
		  void *(*cb)(void *))
{
	struct nb_worker *n = nb_malloc(sizeof(struct nb_worker));
	memset(n, 0, sizeof(struct nb_worker));

	n->id = workers->count;
	n->db.dif = dif;
	n->db.priv = NULL;

	n->key = kif;
	n->key->init(&n->keyv, distif);

	nb_history_init(&n->history, history_max);
	nb_workload_init_from(&n->workload, workload);

	if (pthread_create(&n->tid, NULL, cb, (void*)n) == -1) {
		n->key->free(&n->keyv);
		nb_history_free(&n->history);
		free(n);
		return NULL;
	}

	if (workers->head == NULL)
		workers->head = n;
	else
		workers->tail->next = n;
	workers->tail = n;
	workers->count++;
	return n;
}

void nb_workers_join(struct nb_workers *workers)
{
	struct nb_worker *c = workers->head;
	while (c) {
		void *ret = NULL;
		pthread_join(c->tid, &ret);
		c = c->next;
	}
}
