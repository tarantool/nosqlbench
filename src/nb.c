
/*
 * Copyright (C) 2011 Mail.RU
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include <tnt.h>
#include <tnt_net.h>

#include <nb_queue.h>
#include <nb_stat.h>
#include <nb_func.h>
#include <nb_test.h>
#include <nb_opt.h>
#include <nb.h>
#include <nb_cb.h>
#include <nb_thread.h>
#include <nb_data.h>
#include <nb_plot.h>

void
nb_init(struct nb *bench, struct nb_funcs *funcs,
	struct nb_opt *opt)
{
	bench->funcs = funcs;
	bench->opt = opt;
	nb_test_init(&bench->tests);
}

void
nb_free(struct nb *bench)
{
	nb_test_free(&bench->tests);
}

static void
nb_set_std(struct nb *bench)
{
	struct nb_func *f;
	struct nb_test *t;

	f = nb_func_match(bench->funcs, "tnt-insert");
	t = nb_test_add(&bench->tests, f);
	nb_test_buf_add(t, 32);
	nb_test_buf_add(t, 64);
	nb_test_buf_add(t, 128);

	f = nb_func_match(bench->funcs, "tnt-insert-ret");
	t = nb_test_add(&bench->tests, f);
	nb_test_buf_add(t, 32);
	nb_test_buf_add(t, 64);
	nb_test_buf_add(t, 128);

	f = nb_func_match(bench->funcs, "tnt-update");
	t = nb_test_add(&bench->tests, f);
	nb_test_buf_add(t, 32);
	nb_test_buf_add(t, 64);
	nb_test_buf_add(t, 128);

	f = nb_func_match(bench->funcs, "tnt-update-ret");
	t = nb_test_add(&bench->tests, f);
	nb_test_buf_add(t, 32);
	nb_test_buf_add(t, 64);
	nb_test_buf_add(t, 128);

	f = nb_func_match(bench->funcs, "tnt-select");
	t = nb_test_add(&bench->tests, f);
	nb_test_buf_add(t, 0);
}

static void
nb_set_std_memcache(struct nb *bench)
{
	struct nb_func *f;
	struct nb_test *t;

	f = nb_func_match(bench->funcs, "memcache-set");
	t = nb_test_add(&bench->tests, f);
	nb_test_buf_add(t, 32);
	nb_test_buf_add(t, 64);
	nb_test_buf_add(t, 128);

	f = nb_func_match(bench->funcs, "memcache-get");
	t = nb_test_add(&bench->tests, f);
	nb_test_buf_add(t, 32);
	nb_test_buf_add(t, 64);
	nb_test_buf_add(t, 128);
}

static int
nb_prepare(struct nb *bench)
{
	/* using specified tests, if supplied */
	if (bench->opt->tests_count) {
		struct nb_opt_test *t;
		STAILQ_FOREACH(t, &bench->opt->tests, next) {
			struct nb_func *func = 
				nb_func_match(bench->funcs, t->test);
			if (func == NULL) {
				printf("unknown test: \"%s\", try --test-list\n", t->test);
				return -1;
			}
			nb_test_add(&bench->tests, func);
		}
		struct nb_opt_buf *buf;
		struct nb_test *test;
		STAILQ_FOREACH(buf, &bench->opt->bufs, next) {
			STAILQ_FOREACH(test, &bench->tests.list, next) {
				nb_test_buf_add(test, buf->buf);
			}
		}
	} else {
		if (bench->opt->std) 
			nb_set_std(bench);
		else
		if (bench->opt->std_memcache) 
			nb_set_std_memcache(bench);
	}
	return 0;
}

void
nb_info(struct nb *bench)
{
	struct nb_test *t;
	STAILQ_FOREACH(t, &bench->tests.list, next) {
		printf("%s\n", t->func->name);
		struct nb_test_buf *b;
		STAILQ_FOREACH(b, &t->list, next) {
			printf(" [%4d] ", b->buf);
			printf("%.2f rps, %.2f sec ",
				b->stat.rps, ((float)b->stat.tm / 1000));
			printf("(%.2f rps, %.2f sec)",
				b->stat_es.rps, ((float)b->stat_es.tm / 1000));
			printf("\n");
		}
		if (t->integral) {
			printf(" [intg] %.2f (esync: %.2f) ", t->integral, t->integral_es);
			double val, perc;
			if (t->integral >= t->integral_es) {
				val = t->integral - t->integral_es;
				perc = (val / t->integral) * 100;
			} else {
				val = t->integral_es - t->integral;
				perc = (val / t->integral_es) * 100;
			}
			printf("%.2f%%\n", perc);
		}
	}
}

void
nb_info_int(struct nb *bench)
{
	struct nb_test *t;
	STAILQ_FOREACH(t, &bench->tests.list, next) {
		printf("%s\t%.2f\t%.2f\t", t->func->name, t->integral, t->integral_es);
		if (t->integral == 0.0 ) {
			printf("0.0");
		} else
		if (t->integral >= t->integral_es) {
			double val = t->integral - t->integral_es;
			double perc = (val / t->integral) * 100;
			printf("%.2f", perc);
		} else {
			double val = t->integral_es - t->integral;
			double perc = (val / t->integral_es) * 100;
			printf("%.2f", perc);
		}
		printf("\n");
	}
}

static int run = 0;
static pthread_barrier_t barrier;

static void*
nb_cb(struct nb_thread *t)
{
	struct tnt_stream tnt;
	if (tnt_net(&tnt) == NULL) {
		printf("tnt_stream_net() failed\n");
		return NULL;
	}
	tnt_set(&tnt, TNT_OPT_HOSTNAME, t->nb->opt->host);
	tnt_set(&tnt, TNT_OPT_PORT, t->nb->opt->port);
	tnt_set(&tnt, TNT_OPT_SEND_BUF, t->nb->opt->sbuf);
	tnt_set(&tnt, TNT_OPT_RECV_BUF, t->nb->opt->rbuf);
	if (tnt_init(&tnt) == -1) {
		tnt_stream_free(&tnt);
		return NULL;
	}

	if (tnt_connect(&tnt) == -1) {
		printf("tnt_connect() failed: %s\n", tnt_strerror(&tnt));
		tnt_stream_free(&tnt);
		return NULL;
	}
	while (1) {
		/* waiting for ready */
		pthread_barrier_wait(&barrier);
		if (!run)
			break;
		/* calling test */
		struct nb_func_arg a = { &tnt, t->idx, t->buf->buf, t->nb->opt->per,
			                 &t->stat };
		t->test->func->func(&a);
		/* waiting test to finish */
		pthread_barrier_wait(&barrier);
	}
	tnt_stream_free(&tnt);
	return NULL;
}

void
nb_run(struct nb *bench)
{
	if (nb_prepare(bench) == -1)
		return;

	struct nb_stat *stats =
		malloc(sizeof(struct nb_stat) * bench->opt->rep);
	if (stats == NULL) {
		printf("memory allocation failed\n");
		return;
	}
	struct nb_stat *stats_es =
		malloc(sizeof(struct nb_stat) * bench->opt->rep);
	if (stats_es == NULL) {
		free(stats);
		printf("memory allocation failed\n");
		return;
	}

	struct nb_threads threads;
	nb_threads_init(&threads);

	if (pthread_barrier_init(&barrier,
		NULL, bench->opt->threads + 1 /* main */) != 0) {
		free(stats);
		free(stats_es);
		printf("failed to init barrier\n");
		return;
	}
	run = 1;

	/* creating threads and waiting for ready */
	if (nb_threads_create(&threads, bench->opt->threads,
		bench, (nb_threadf_t)nb_cb) == -1) {
		free(stats);
		free(stats_es);
		printf("failed to create threads\n");
		return;
	}

	int rc;
	struct nb_test *t;
	STAILQ_FOREACH(t, &bench->tests.list, next) {
		printf("%s\n", t->func->name);
		fflush(stdout);

		struct nb_test_buf *b;
		STAILQ_FOREACH(b, &t->list, next) {
			memset(stats, 0, sizeof(struct nb_stat) * bench->opt->rep);
			memset(stats_es, 0, sizeof(struct nb_stat) * bench->opt->rep);
			uint32_t rep;
			for (rep = 0 ; rep < bench->opt->rep ; rep++) {
				printf(" [%4d] ", b->buf);
				fflush(stdout);

				/* giving current test, buffer to threads */
				nb_threads_set(&threads, t, b);

				/* starting timer */
				nb_stat_start(&stats[rep], bench->opt->count);

				/* waiting threads to connect, start test */
				rc = pthread_barrier_wait(&barrier);
				assert(rc == 0 || rc == PTHREAD_BARRIER_SERIAL_THREAD);

				/* waiting threads to finish test */
				rc = pthread_barrier_wait(&barrier);
				assert(rc == 0 || rc == PTHREAD_BARRIER_SERIAL_THREAD);

				nb_stat_stop(&stats[rep]);

				/* calculating avg value over all threads */
				uint32_t i;
				for (i = 0 ; i < bench->opt->threads ; i++) {
					struct nb_thread *t = &threads.threads[i];
					stats_es[rep].tm  += t->stat.tm;
					stats_es[rep].rps += t->stat.rps;
				}
				stats_es[rep].tm /= bench->opt->threads;

				printf("%.2f rps, %.2f sec ",
					stats[rep].rps, ((float)stats[rep].tm / 1000));

				if (bench->opt->esync)
					printf("(%.2f rps, %.2f sec)",
						stats_es[rep].rps, ((float)stats_es[rep].tm / 1000));
				printf("\n");
				fflush(stdout);

				b->stat.tm  += stats[rep].tm;
				b->stat.rps += stats[rep].rps;
				b->stat_es.tm  += stats_es[rep].tm;
				b->stat_es.rps += stats_es[rep].rps;
			}

			/* calculating avg value for buffer */
			b->stat.rps /= bench->opt->rep;
			b->stat.tm  /= bench->opt->rep;
			b->stat_es.rps /= bench->opt->rep;
			b->stat_es.tm  /= bench->opt->rep;

			printf("        %.2f rps, %.2f sec ",
				b->stat.rps, ((float)b->stat.tm / 1000));
			if (bench->opt->esync)
				printf("(%.2f rps, %.2f sec)",
					b->stat_es.rps, ((float)b->stat_es.tm / 1000));
			printf("\n");
			fflush(stdout);
		}

		/* calculating plot integral */
		nb_test_integrate(t);
		printf("  [sum] %.2f rps ", t->integral);
		if (bench->opt->esync)
			printf(" (%.2f rps)", t->integral_es);
		printf("\n");
		fflush(stdout);
	}

	/* finalizing threads */
	run = 0;
	rc = pthread_barrier_wait(&barrier);
	assert(rc == 0 || rc == PTHREAD_BARRIER_SERIAL_THREAD);
	pthread_barrier_destroy(&barrier);

	nb_threads_join(&threads);
	nb_threads_free(&threads);

	free(stats);
	free(stats_es);

	if (bench->opt->data == NB_OPT_DATA_SAVE)
		nb_data_save(bench);
	if (bench->opt->plot)
		nb_plot_gen(bench);
}
