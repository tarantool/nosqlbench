#ifndef NB_WORKER_H_INCLUDED
#define NB_WORKER_H_INCLUDED

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

struct nb_worker {
	int id;
	struct nb_db db;
	struct nb_key_if *key;
	struct nb_key keyv;
	struct nb_workload workload;
	struct nb_history history;
	pthread_t tid;
	struct nb_worker *next;
};

struct nb_workers {
	struct nb_worker *head, *tail;
	volatile int count;
};

void nb_workers_init(struct nb_workers *workers);
void nb_workers_free(struct nb_workers *workers);

struct nb_worker*
nb_workers_create(struct nb_workers *workers, struct nb_db_if *dif,
		  struct nb_key_if *kif,
		  struct nb_key_distribution_if *distif,
		  struct nb_workload *workload, int history_max,
		  void *(*cb)(void *));

void nb_workers_join(struct nb_workers *workers);

#endif
