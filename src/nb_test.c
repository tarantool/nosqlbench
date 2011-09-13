
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

#include <nb_stat.h>
#include <nb_func.h>
#include <nb_test.h>

void
nb_test_init(struct nb_tests *tests)
{
	tests->count = 0;
	STAILQ_INIT(&tests->list);
}

void
nb_test_free(struct nb_tests *tests)
{
	struct nb_test *t, *tnext;
	STAILQ_FOREACH_SAFE(t, &tests->list, next, tnext) {
		struct nb_test_buf *b, *bnext;
		STAILQ_FOREACH_SAFE(b, &t->list, next, bnext) {
			free(b);
		}
		free(t);
	}
}

struct nb_test*
nb_test_add(struct nb_tests *tests, struct nb_func *func)
{
	struct nb_test *test =
		malloc(sizeof(struct nb_test));
	if (test == NULL)
		return NULL;
	test->func = func;
	test->integral = 0.0;
	test->integral_es = 0.0;
	tests->count++;
	STAILQ_INIT(&test->list);
	STAILQ_INSERT_TAIL(&tests->list, test, next);
	return test;
}

struct nb_test_buf*
nb_test_buf_add(struct nb_test *test, int buf)
{
	struct nb_test_buf *b =
		malloc(sizeof(struct nb_test_buf));
	if (b == NULL)
		return NULL;
	b->buf = buf;
	memset(&b->stat, 0, sizeof(b->stat));
	memset(&b->stat_es, 0, sizeof(b->stat_es));
	test->count++;
	STAILQ_INSERT_TAIL(&test->list, b, next);
	return b;
}

char*
nb_test_buf_list(struct nb_test *test)
{
	int pos = 0;
	int first = 1;
	static char list[8096];
	struct nb_test_buf *b;
	STAILQ_FOREACH(b, &test->list, next) {
		if (first) {
			pos += snprintf(list + pos, sizeof(list) - pos, "%d", b->buf);
			first = 0;
		} else {
			pos += snprintf(list + pos, sizeof(list) - pos, ", %d", b->buf);
		}
	}
	return list;
}

int
nb_test_buf_max(struct nb_test *test)
{
	uint32_t max;
	if (test->count)
		max = STAILQ_FIRST(&test->list)->buf;
	struct nb_test_buf *b;
	STAILQ_FOREACH(b, &test->list, next) {
		if (b->buf > max)
			max = b->buf;
	}
	return max;
}

int
nb_test_buf_min(struct nb_test *test)
{
	uint32_t min;
	if (test->count)
		min = STAILQ_FIRST(&test->list)->buf;
	struct nb_test_buf *b;
	STAILQ_FOREACH(b, &test->list, next) {
		if (b->buf < min)
			min = b->buf;
	}
	return min;
}

static void
nb_test_integratef(struct nb_test *test, double buf, double *rps, double *rpses)
{
	/* matching lower bound */
	struct nb_test_buf *b, *start = NULL;
	STAILQ_FOREACH(b, &test->list, next)
		if (b->buf <= buf && (start == NULL || b->buf >= start->buf))
			start = b;
	if (start == NULL)
		return;

	/* interpolating rps value */
	*rps = start->stat.rps +
		((buf - start->buf) / (STAILQ_NEXT(start, next)->buf - start->buf)) *
			(STAILQ_NEXT(start, next)->stat.rps - start->stat.rps);
	*rpses = start->stat_es.rps +
		((buf - start->buf) / (STAILQ_NEXT(start, next)->buf - start->buf)) *
			(STAILQ_NEXT(start, next)->stat_es.rps - start->stat_es.rps);
}

void
nb_test_integrate(struct nb_test *test)
{
	if (test->count <= 1)
		return;
	test->integral = 0.0;
	double a = nb_test_buf_min(test),
	       b = nb_test_buf_max(test);
	double acq = 1000;
	double interval = (b - a) / acq;
	int i;
	for (i = 1; i<= acq ; i++) {
		double rps, rpses;
		rps = rpses = 0.0;
		nb_test_integratef(test, a + interval * (i - 0.5), &rps, &rpses);
		test->integral += rps;
		test->integral_es += rpses;
	}
	test->integral *= interval;
	test->integral_es *= interval;
}
