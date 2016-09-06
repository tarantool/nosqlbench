
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

#include "nosqlbench.h"
#include "async_io.h"
#include "nb_histogram.h"

extern struct nb nb;

extern volatile sig_atomic_t nb_signaled;

struct io_user_data {
	struct nb_worker *worker;
	struct nb_request *request;
	enum nb_request_type prev_type;
};

static void
nb_worker_init(void) {
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
}

static int io_write_impl(struct io_user_data *ud)
{
	struct nb_worker *worker = ud->worker;
	if (nb.is_done)
		return 1;
	/**
	 * The request can be unset if it is first call of io_write
	 * or if all requests already sent and need to rewind list of
	 * requests.
	 */
	if (ud->request == NULL) {
		nb_workload_reset(&worker->workload);
		ud->request = nb_workload_fetch(&worker->workload);
		if (ud->request == NULL)
			return 1;
	} else {
		/* If the previous request deleted a tuple then reinsert it. */
		if (ud->prev_type == NB_DELETE) {
			nb.db->replace(&worker->db, &worker->keyv);
			worker->workload.requested++;
			ud->prev_type = NB_INSERT;
			nb_history_add(&worker->history, RT_WRITE);
			return 0;
		}
	}
	worker->key->generate(&worker->keyv, worker->workload.count);
	ud->request->_do(&worker->db, &worker->keyv);
	ud->request->requested++;
	worker->workload.requested++;
	nb_history_add(&worker->history, ud->request->type ==
		       NB_SELECT ? RT_READ : RT_WRITE);
	ud->prev_type = ud->request->type;
	ud->request = nb_workload_fetch(&worker->workload);
	return 0;
}

static void *io_write(struct async_io_object *io_obj, size_t *size)
{
	struct io_user_data *ud;
	ud = (struct io_user_data *)async_io_get_user_data(io_obj);
	struct nb_worker *worker = ud->worker;
	if (io_write_impl(ud)) {
		async_io_finish(io_obj);
		return NULL;
	}
	return nb.db->get_buf(&worker->db, size);
}

static int io_msg_len(struct async_io_object *io_obj, void *buf, size_t size)
{
	(void)io_obj;
	return nb.db->msg_len(buf, size);
}

static void process_latency(void *lat_arg, uint64_t latency)
{
	struct io_user_data *ud;
	ud = (struct io_user_data *)lat_arg;
	struct nb_worker *worker = ud->worker;
	nb_histogram_add(worker->total_hist, latency);
	nb_histogram_add(worker->period_hist, latency);
}

static int io_recv_from_buf(struct async_io_object *io_obj, char *buf,
			    size_t size, size_t *off)
{
	struct io_user_data *ud;
	ud = (struct io_user_data *)async_io_get_user_data(io_obj);
	struct nb_worker *worker = ud->worker;
	uint64_t latency = 0;
	int rc = nb.db->recv_from_buf(buf, size, off, &latency);
	process_latency(ud, latency);

	nb_history_avg(&worker->history);
	pthread_mutex_lock(&nb.stats.lock_stats);
	nb_statistics_set(&nb.stats, worker->id, &worker->history.Savg);
	pthread_mutex_unlock(&nb.stats.lock_stats);
	return rc;
}

static void *nb_worker(void *ptr)
{
	struct nb_worker *worker = ptr;

	nb_worker_init();

	nb.db->init(&worker->db, nb.opts.value_size);
	if (nb.db->connect(&worker->db, &nb.opts) == -1)
		return NULL;

	struct io_user_data userdata;
	userdata.worker = worker;
	userdata.request = NULL;
	userdata.prev_type = NB_INSERT;
	if (nb.opts.request_batch_count) {
		int rc = 0;
		do {
			int i = 0;
			for (; i < nb.opts.request_batch_count; ++i) {
				rc = io_write_impl(&userdata);
				if (rc)
					break;
			}
			worker->db.dif->recv(&worker->db, i, NULL, process_latency, &userdata);
			nb_history_add(&worker->history, RT_MISS);

			nb_history_avg(&worker->history);
			pthread_mutex_lock(&nb.stats.lock_stats);
			nb_statistics_set(&nb.stats, worker->id, &worker->history.Savg);
			pthread_mutex_unlock(&nb.stats.lock_stats);
		} while (!rc);
	} else {
		struct async_io_if io_if = {io_msg_len, io_write, io_recv_from_buf};
		struct async_io_object *io_object;
		int sock = nb.db->get_fd(&worker->db);
		if (nb.opts.rps != 0) {
			io_object = async_io_new_rps(sock, &io_if, nb.opts.rps,
						     &userdata);
		} else {
			io_object = async_io_new(nb.db->get_fd(&worker->db), &io_if,
						 &userdata);
		}
		if (io_object == NULL)
			goto error;
		async_io_start(io_object);

		async_io_delete(io_object);
	}
error:
	return NULL;
}

static void nb_create(void)
{
	int max = (nb.opts.threads_policy == NB_THREADS_ATONCE) ?
		   nb.opts.threads_max :
		   nb.opts.threads_start;
	int i;
	for (i = 0; i < max; i++)
		nb_workers_create(&nb.workers,
				  nb.db,
				  nb.key,
				  nb.key_dist,
				  &nb.workload, 
				  nb.opts.history_per_batch,
				  nb_worker);
}

static void nb_create_step(void)
{
	if (nb.opts.threads_policy == NB_THREADS_ATONCE)
		return;
	if (nb.workers.count < nb.opts.threads_max) {
		int top = 0;
		if (nb.workers.count + nb.opts.threads_increment > nb.opts.threads_max)
			top = nb.opts.threads_max - nb.workers.count;
		else
			top = nb.opts.threads_increment;
		printf("\nAdd %d new thread(s) to benchmarking\n\n", top);
		for (int i = 0; i < top; i++)
			nb_workers_create(&nb.workers,
					  nb.db,
					  nb.key,
					  nb.key_dist,
					  &nb.workload, 
					  nb.opts.history_per_batch,
					  nb_worker);

		pthread_mutex_lock(&nb.stats.lock_stats);
		nb_statistics_resize(&nb.stats, nb.workers.count);
		pthread_mutex_unlock(&nb.stats.lock_stats);
	}
}

static void nb_report(void)
{
	pthread_mutex_lock(&nb.stats.lock_stats);
	nb_statistics_report(&nb.stats, nb.workers.count, nb.tick);
	if (nb.report->report)
		nb.report->report();
	pthread_mutex_unlock(&nb.stats.lock_stats);
}


static void nb_limit(void)
{
	if (nb_signaled) {
		nb.is_done = 1;
		return;
	}
	switch (nb.opts.benchmark_policy) {
	case NB_BENCHMARK_NOLIMIT:
		return;
	case NB_BENCHMARK_THREADLIMIT:
		if (nb.workers.count >= nb.opts.threads_max)
			nb.is_done = 1;
		break;
	case NB_BENCHMARK_TIMELIMIT:
		if (nb_period_equal(nb.opts.time_limit))
			nb.is_done = 1;
		break;
	}
}

void nb_engine(void)
{
	nb_create();

	while (!nb.is_done) {
		sleep(1);
		nb_limit();

		if (nb_period_equal(nb.opts.report_interval))
			nb_report();

		if (nb_period_equal(nb.opts.threads_interval))
			nb_create_step();

		nb_tick();
	}

	nb_workers_join(&nb.workers);

	nb_statistics_final(&nb.stats);

	if (nb.report->report_final)
		nb.report->report_final();

	if (nb.opts.csv_file)
		nb_statistics_csv(&nb.stats, nb.opts.csv_file);

	pthread_mutex_destroy(&nb.stats.lock_stats);
}
