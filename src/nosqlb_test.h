#ifndef NOSQLB_TEST_H_INCLUDED
#define NOSQLB_TEST_H_INCLUDED

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

struct nosqlb_test_buf {
	int buf;
	struct nosqlb_stat avg;
	STAILQ_ENTRY(nosqlb_test_buf) next;
};

struct nosqlb_test {
	struct nosqlb_func *func;
	int count;
	STAILQ_HEAD(,nosqlb_test_buf) list;
	STAILQ_ENTRY(nosqlb_test) next;
};

struct nosqlb_tests {
	int count;
	STAILQ_HEAD(,nosqlb_test) list;
};

void nosqlb_test_init(struct nosqlb_tests *tests);
void nosqlb_test_free(struct nosqlb_tests *tests);

struct nosqlb_test*
nosqlb_test_add(struct nosqlb_tests *tests, struct nosqlb_func *func);
void nosqlb_test_buf_add(struct nosqlb_test *test, int buf);

char *nosqlb_test_buf_list(struct nosqlb_test *test);
int nosqlb_test_buf_max(struct nosqlb_test *test);
int nosqlb_test_buf_min(struct nosqlb_test *test);

#endif /* NOSQLB_TEST_H_INCLUDED */
