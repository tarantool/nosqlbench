#ifndef NB_TEST_H_INCLUDED
#define NB_TEST_H_INCLUDED

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

struct nb_test_buf {
	uint32_t buf;
	struct nb_stat stat;
	struct nb_stat stat_es;
	STAILQ_ENTRY(nb_test_buf) next;
};

struct nb_test {
	struct nb_func *func;
	uint32_t count;
	double integral;
	double integral_es;
	STAILQ_HEAD(,nb_test_buf) list;
	STAILQ_ENTRY(nb_test) next;
};

struct nb_tests {
	uint32_t count;
	STAILQ_HEAD(,nb_test) list;
};

void nb_test_init(struct nb_tests *tests);
void nb_test_free(struct nb_tests *tests);

struct nb_test*
nb_test_add(struct nb_tests *tests, struct nb_func *func);

struct nb_test_buf*
nb_test_buf_add(struct nb_test *test, int buf);

char *nb_test_buf_list(struct nb_test *test);
int nb_test_buf_max(struct nb_test *test);
int nb_test_buf_min(struct nb_test *test);

void nb_test_integrate(struct nb_test *test);

#endif /* NB_TEST_H_INCLUDED */
