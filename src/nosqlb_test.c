
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

void
nosqlb_test_init(struct nosqlb_tests *tests)
{
	tests->count = 0;
	STAILQ_INIT(&tests->list);
}

void
nosqlb_test_free(struct nosqlb_tests *tests)
{
	struct nosqlb_test *t, *tnext;
	STAILQ_FOREACH_SAFE(t, &tests->list, next, tnext) {
		struct nosqlb_test_buf *b, *bnext;
		STAILQ_FOREACH_SAFE(b, &t->list, next, bnext) {
			free(b);
		}
		free(t);
	}
}

struct nosqlb_test*
nosqlb_test_add(struct nosqlb_tests *tests, struct nosqlb_func *func)
{
	struct nosqlb_test *test =
		malloc(sizeof(struct nosqlb_test));
	if (test == NULL)
		return NULL;
	test->func = func;
	tests->count++;
	STAILQ_INIT(&test->list);
	STAILQ_INSERT_TAIL(&tests->list, test, next);
	return test;
}

void
nosqlb_test_buf_add(struct nosqlb_test *test, int buf)
{
	struct nosqlb_test_buf *b =
		malloc(sizeof(struct nosqlb_test_buf));
	if (b == NULL)
		return;
	b->buf = buf;
	memset(&b->stat, 0, sizeof(b->stat));
	test->count++;
	STAILQ_INSERT_TAIL(&test->list, b, next);
}

char*
nosqlb_test_buf_list(struct nosqlb_test *test)
{
	int pos = 0;
	int first = 1;
	static char list[4096];
	struct nosqlb_test_buf *b;
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
nosqlb_test_buf_max(struct nosqlb_test *test)
{
	int max;
	if (test->count)
		max = STAILQ_FIRST(&test->list)->buf;
	struct nosqlb_test_buf *b;
	STAILQ_FOREACH(b, &test->list, next) {
		if (b->buf > max)
			max = b->buf;
	}
	return max;
}

int
nosqlb_test_buf_min(struct nosqlb_test *test)
{
	int min;
	if (test->count)
		min = STAILQ_FIRST(&test->list)->buf;
	struct nosqlb_test_buf *b;
	STAILQ_FOREACH(b, &test->list, next) {
		if (b->buf < min)
			min = b->buf;
	}
	return min;
}
