
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

#include <tnt.h>

#include <nosqlb_stat.h>
#include <nosqlb_func.h>
#include <nosqlb_test.h>
#include <nosqlb_opt.h>
#include <nosqlb.h>
#include <nosqlb_cb.h>
#include <nosqlb_plot.h>

int
nosqlb_init(struct nosqlb *bench, struct nosqlb_funcs *funcs,
	    struct nosqlb_opt *opt)
{
	bench->funcs = funcs;
	bench->opt = opt;

	nosqlb_test_init(&bench->tests);

	bench->t = tnt_alloc();
	if (bench->t == NULL)
		return -1;

	tnt_set(bench->t, TNT_OPT_PROTO, opt->proto);
	tnt_set(bench->t, TNT_OPT_HOSTNAME, opt->host);
	tnt_set(bench->t, TNT_OPT_PORT, opt->port);
	tnt_set(bench->t, TNT_OPT_SEND_BUF, opt->sbuf);
	tnt_set(bench->t, TNT_OPT_RECV_BUF, opt->rbuf);

	if (tnt_init(bench->t) == -1)
		return -1;
	return 0;
}

void
nosqlb_free(struct nosqlb *bench)
{
	nosqlb_test_free(&bench->tests);
	tnt_free(bench->t);
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

int
nosqlb_connect(struct nosqlb *bench)
{
	return tnt_connect(bench->t);
}

void
nosqlb_run(struct nosqlb *bench)
{
	/* using specified tests, if supplied */
	if (bench->opt->tests_count) {
		struct nosqlb_opt_arg *arg;
		STAILQ_FOREACH(arg, &bench->opt->tests, next) {
			struct nosqlb_func *func = 
				nosqlb_func_match(bench->funcs, arg->arg);
			if (func == NULL) {
				printf("unknown test: \"%s\", try --test-list\n", arg->arg);
				return;
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

	struct nosqlb_stat *stats =
		malloc(sizeof(struct nosqlb_stat) * bench->opt->reps);
	if (stats == NULL)
		return;

	struct nosqlb_test *t;
	STAILQ_FOREACH(t, &bench->tests.list, next) {
		if (bench->opt->color)
			printf("\033[22;33m%s\033[0m\n", t->func->name);
		else
			printf("%s\n", t->func->name);
		fflush(stdout);

		struct nosqlb_test_buf *b;
		STAILQ_FOREACH(b, &t->list, next) {
			printf("  >>> [%d] ", b->buf);
			memset(stats, 0, sizeof(struct nosqlb_stat) *
				bench->opt->reps);
			fflush(stdout);

			int r;
			for (r = 0 ; r < bench->opt->reps ; r++) {
				t->func->func(bench->t, b->buf, bench->opt->count, &stats[r]);
				printf("<%.2f %.2f> ", stats[r].rps, (float)stats[r].tm / 1000);
				fflush(stdout);
			}

			float rps = 0.0;
			unsigned long long tm = 0;
			for (r = 0 ; r < bench->opt->reps ; r++) {
				rps += stats[r].rps;
				tm += stats[r].tm;
			}

			b->avg.rps   = rps / bench->opt->reps;
			b->avg.tm    = (float)tm / 1000 / bench->opt->reps;
			b->avg.start = 0;
			b->avg.count = 0;

			printf("\n");
			if (bench->opt->color) 
				printf("  <<< (avg time \033[22;35m%.2f\033[0m sec): \033[22;32m%.2f\033[0m rps\n", 
					b->avg.tm, b->avg.rps);
			else
				printf("  <<< (avg time %.2f sec): %.2f rps\n", 
					b->avg.tm, b->avg.rps);
		}
	}

	free(stats);
	if (bench->opt->plot)
		nosqlb_plot(bench);
}
