
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
