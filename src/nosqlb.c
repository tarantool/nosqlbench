
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <tnt.h>

#include <nosqlb_stat.h>
#include <nosqlb_func.h>
#include <nosqlb_test.h>
#include <nosqlb_opt.h>
#include <nosqlb.h>
#include <nosqlb_cb.h>
#include <nosqlb_thread.h>
#include <nosqlb_plot.h>

void
nosqlb_init(struct nosqlb *bench, struct nosqlb_funcs *funcs,
	    struct nosqlb_opt *opt)
{
	bench->funcs = funcs;
	bench->opt = opt;
	nosqlb_test_init(&bench->tests);
}

void
nosqlb_free(struct nosqlb *bench)
{
	nosqlb_test_free(&bench->tests);
}

static void
nosqlb_set_std(struct nosqlb *bench)
{
	struct nosqlb_func *f;
	struct nosqlb_test *t;

	f = nosqlb_func_match(bench->funcs, "tnt-insert");
	t = nosqlb_test_add(&bench->tests, f);
	nosqlb_test_buf_add(t, 32);
	nosqlb_test_buf_add(t, 64);
	nosqlb_test_buf_add(t, 128);

	f = nosqlb_func_match(bench->funcs, "tnt-insert-ret");
	t = nosqlb_test_add(&bench->tests, f);
	nosqlb_test_buf_add(t, 32);
	nosqlb_test_buf_add(t, 64);
	nosqlb_test_buf_add(t, 128);

	f = nosqlb_func_match(bench->funcs, "tnt-update");
	t = nosqlb_test_add(&bench->tests, f);
	nosqlb_test_buf_add(t, 32);
	nosqlb_test_buf_add(t, 64);
	nosqlb_test_buf_add(t, 128);

	f = nosqlb_func_match(bench->funcs, "tnt-update-ret");
	t = nosqlb_test_add(&bench->tests, f);
	nosqlb_test_buf_add(t, 32);
	nosqlb_test_buf_add(t, 64);
	nosqlb_test_buf_add(t, 128);

	f = nosqlb_func_match(bench->funcs, "tnt-select");
	t = nosqlb_test_add(&bench->tests, f);
	nosqlb_test_buf_add(t, 0);
}

static void
nosqlb_set_std_memcache(struct nosqlb *bench)
{
	struct nosqlb_func *f;
	struct nosqlb_test *t;

	f = nosqlb_func_match(bench->funcs, "memcache-set");
	t = nosqlb_test_add(&bench->tests, f);
	nosqlb_test_buf_add(t, 32);
	nosqlb_test_buf_add(t, 64);
	nosqlb_test_buf_add(t, 128);

	f = nosqlb_func_match(bench->funcs, "memcache-get");
	t = nosqlb_test_add(&bench->tests, f);
	nosqlb_test_buf_add(t, 32);
	nosqlb_test_buf_add(t, 64);
	nosqlb_test_buf_add(t, 128);
}

static int
nosqlb_prepare(struct nosqlb *bench)
{
	/* using specified tests, if supplied */
	if (bench->opt->tests_count) {
		struct nosqlb_opt_arg *arg;
		STAILQ_FOREACH(arg, &bench->opt->tests, next) {
			struct nosqlb_func *func = 
				nosqlb_func_match(bench->funcs, arg->arg);
			if (func == NULL) {
				printf("unknown test: \"%s\", try --test-list\n", arg->arg);
				return -1;
			}
			nosqlb_test_add(&bench->tests, func);
		}
		struct nosqlb_test *test;
		STAILQ_FOREACH(arg, &bench->opt->bufs, next) {
			STAILQ_FOREACH(test, &bench->tests.list, next) {
				nosqlb_test_buf_add(test, atoi(arg->arg));
			}
		}
	} else {
		if (bench->opt->std) 
			nosqlb_set_std(bench);
		else
		if (bench->opt->std_memcache) 
			nosqlb_set_std_memcache(bench);
	}
	return 0;
}

static void*
nosqlb_cb(struct nosqlb_thread *t)
{
	t->t = tnt_alloc();
	if (t->t == NULL) {
		printf("tnt_alloc() failed\n");
		return NULL;
	}

	tnt_set(t->t, TNT_OPT_PROTO, t->nosqlb->opt->proto);
	tnt_set(t->t, TNT_OPT_HOSTNAME, t->nosqlb->opt->host);
	tnt_set(t->t, TNT_OPT_PORT, t->nosqlb->opt->port);
	tnt_set(t->t, TNT_OPT_SEND_BUF, t->nosqlb->opt->sbuf);
	tnt_set(t->t, TNT_OPT_RECV_BUF, t->nosqlb->opt->rbuf);
	tnt_set(t->t, TNT_OPT_TMOUT_CONNECT, 8);
	if (tnt_init(t->t) == -1)
		return NULL;

	if (tnt_connect(t->t) == -1) {
		printf("tnt_connect() failed: %s\n", tnt_strerror(t->t));
		return NULL;
	}

	/* waiting at the start point */
	nosqlb_threads_barrier_ready();

	/* calling test */
	t->test->func->func(t->t, t->idx, t->buf->buf, t->nosqlb->opt->per,
			    &t->stat);
	tnt_free(t->t);
	return NULL;
}

void
nosqlb_run(struct nosqlb *bench)
{
	if (nosqlb_prepare(bench) == -1)
		return;

	struct nosqlb_stat *stats =
		malloc(sizeof(struct nosqlb_stat) * bench->opt->rep);
	if (stats == NULL) {
		printf("memory allocation failed\n");
		return;
	}

	struct nosqlb_test *t;
	STAILQ_FOREACH(t, &bench->tests.list, next) {
		printf("%s\n", t->func->name);
		fflush(stdout);

		struct nosqlb_test_buf *b;
		STAILQ_FOREACH(b, &t->list, next) {
			memset(stats, 0, sizeof(struct nosqlb_stat) * bench->opt->rep);
			int rep;
			for (rep = 0 ; rep < bench->opt->rep ; rep++) {
				printf(" [%4d] ", b->buf);
				fflush(stdout);

				struct nosqlb_threads threads;
				nosqlb_threads_init(&threads);

				/* creating threads and waiting for ready */
				nosqlb_threads_barrier_up();
				nosqlb_threads_create(&threads, bench->opt->threads,
					 bench, (nosqlb_threadf_t)nosqlb_cb, t, b);
				nosqlb_threads_barrier(&threads);

				/* starting timer */
				nosqlb_stat_start(&stats[rep], bench->opt->count);

				/* starting test */
				nosqlb_threads_barrier_down();
				nosqlb_threads_join(&threads);

				/* calculating statistics for current buf */
				nosqlb_stat_stop(&stats[rep]);
				nosqlb_threads_free(&threads);

				printf("%.2f rps, %.2f sec\n",
					stats[rep].rps, ((float)stats[rep].tm / 1000));
				fflush(stdout);

				b->stat.tm  += stats[rep].tm;
				b->stat.rps += stats[rep].rps;
			}

			b->stat.rps /= bench->opt->rep;
			b->stat.tm  /= bench->opt->rep;
			printf("        %.2f rps, %.2f sec\n",
				b->stat.rps, ((float)b->stat.tm / 1000));
			fflush(stdout);
		}
	}

	free(stats);
	if (bench->opt->plot)
		nosqlb_plot(bench);
}
