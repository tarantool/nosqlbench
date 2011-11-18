
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
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <tnt.h>

#include <nb_queue.h>
#include <nb_stat.h>
#include <nb_func.h>
#include <nb_test.h>
#include <nb_opt.h>
#include <nb.h>
#include <nb_data.h>

#define NB_DATA_MAGIC 0x000BE7C0

static int
nb_data_save_stat(int fd, struct nb_stat *stat) {
	return (write(fd, stat, sizeof(struct nb_stat)) ==
		sizeof(struct nb_stat)) ? 0 : -1;
}

static int
nb_data_save_fp(int fd, double val) {
	return (write(fd, &val, sizeof(val)) == sizeof(val)) ? 0 : -1;
}

static int
nb_data_save_int(int fd, uint32_t val) {
	return (write(fd, &val, sizeof(val)) == sizeof(val)) ? 0 : -1;
}

static int
nb_data_save_sz(int fd, char *sz) {
	ssize_t len = strlen(sz);
	if (nb_data_save_int(fd, len) == -1)
		return -1;
	return (write(fd, sz, len) == len) ? 0 : -1;
}

static int
nb_data_load_stat(int fd, struct nb_stat *stat) {
	return (read(fd, stat, sizeof(struct nb_stat)) ==
		sizeof(*stat)) ? 0 : -1;
}

static int
nb_data_load_fp(int fd, double *val) {
	return (read(fd, val, sizeof(*val)) == sizeof(*val)) ? 0 : -1;
}

static int
nb_data_load_int(int fd, uint32_t *val) {
	return (read(fd, val, sizeof(*val)) == sizeof(*val)) ? 0 : -1;
}

static int
nb_data_load_sz(int fd, char **sz) {
	uint32_t len = 0;
	if (nb_data_load_int(fd, &len) == -1)
		return -1;
	*sz = malloc(len + 1);
	if (*sz == NULL)
		return -1;
	if (read(fd, *sz, len) != (ssize_t)len) {
		free(*sz);
		return -1;
	}
	(*sz)[len] = 0;
	return 0;
}

static int
nb_data_ret(int fd, int ret) {
	close(fd);
	return ret;
}

int
nb_data_save(struct nb *bench)
{
	int fd = open(bench->opt->datap, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	if (fd == -1)
		return -1;
	/* magic */
	uint32_t magic = NB_DATA_MAGIC;
	if (nb_data_save_int(fd, magic) == -1)
		return nb_data_ret(fd, -1);
	/* number of operations */
	if (nb_data_save_int(fd, bench->opt->count) == -1)
		return nb_data_ret(fd, -1);
	/* number of repeats */
	if (nb_data_save_int(fd, bench->opt->rep) == -1)
		return nb_data_ret(fd, -1);
	/* number of threads */
	if (nb_data_save_int(fd, bench->opt->threads) == -1)
		return nb_data_ret(fd, -1);
	/* number of tests */
	if (nb_data_save_int(fd, bench->tests.count) == -1)
		return nb_data_ret(fd, -1);
	/* tests */
	struct nb_test *t;
	STAILQ_FOREACH(t, &bench->tests.list, next) {
		/* test function name */
		if (nb_data_save_sz(fd, t->func->name) == -1)
			return nb_data_ret(fd, -1);
		/* test integral values */
		if (nb_data_save_fp(fd, t->integral) == -1)
			return nb_data_ret(fd, -1);
		if (nb_data_save_fp(fd, t->integral_es) == -1)
			return nb_data_ret(fd, -1);
		/* buffers count */
		if (nb_data_save_int(fd, t->count) == -1)
			return nb_data_ret(fd, -1);
		/* buffers */
		struct nb_test_buf *b;
		STAILQ_FOREACH(b, &t->list, next) {
			/* buffer statistics */
			if (nb_data_save_int(fd, b->buf) == -1)
				return nb_data_ret(fd, -1);
			if (nb_data_save_stat(fd, &b->stat) == -1)
				return nb_data_ret(fd, -1);
			if (nb_data_save_stat(fd, &b->stat_es) == -1)
				return nb_data_ret(fd, -1);
		}
	}
	return nb_data_ret(fd, 0);
}

static int
nb_data_loadto(char *path, struct nb_funcs *funcs,
	       struct nb_tests *tests,
	       uint32_t *reqs,
	       uint32_t *reps, uint32_t *threads)
{
	int fd = open(path, O_RDONLY);
	if (fd == -1)
		return -1;
	/* magic */
	uint32_t magic = 0;
	if (nb_data_load_int(fd, &magic) == -1)
		return nb_data_ret(fd, -1);
	if (magic != NB_DATA_MAGIC)
		return nb_data_ret(fd, -1);
	/* number of operations */
	if (nb_data_load_int(fd, reqs) == -1)
		return nb_data_ret(fd, -1);
	/* number of repeats */
	if (nb_data_load_int(fd, reps) == -1)
		return nb_data_ret(fd, -1);
	/* number of threads */
	if (nb_data_load_int(fd, threads) == -1)
		return nb_data_ret(fd, -1);
	/* number of tests */
	uint32_t count = 0;
	if (nb_data_load_int(fd, &count) == -1)
		return nb_data_ret(fd, -1);
	/* tests */
	uint32_t i, j;
	for (i = 0 ; i < count ; i++) {
		/* creating test object */
		char *name;
		if (nb_data_load_sz(fd, &name) == -1)
			return nb_data_ret(fd, -1);
		struct nb_func *func = 
			nb_func_match(funcs, name);
		free(name);
		if (func == NULL) {
			printf("unknown test: \"%s\"", name);
			return nb_data_ret(fd, -1);
		}
		struct nb_test *test = nb_test_add(tests, func);
		/* test integral values */
		if (nb_data_load_fp(fd, &test->integral) == -1)
			return nb_data_ret(fd, -1);
		if (nb_data_load_fp(fd, &test->integral_es) == -1)
			return nb_data_ret(fd, -1);
		/* test buffers count */
		uint32_t bufs;
		if (nb_data_load_int(fd, &bufs) == -1)
			return nb_data_ret(fd, -1);
		for (j = 0 ; j < bufs ; j++) {
			/* buffer statistics */
			uint32_t bv;
			if (nb_data_load_int(fd, &bv) == -1)
				return nb_data_ret(fd, -1);
			struct nb_test_buf *buf =
				nb_test_buf_add(test, bv);
			if (nb_data_load_stat(fd, &buf->stat) == -1)
				return nb_data_ret(fd, -1);
			if (nb_data_load_stat(fd, &buf->stat_es) == -1)
				return nb_data_ret(fd, -1);
		}
	}
	return nb_data_ret(fd, 0);
}

int
nb_data_load(struct nb *bench)
{
	return nb_data_loadto(bench->opt->datap,
			      bench->funcs,
			      &bench->tests,
			      &bench->opt->count,
			      &bench->opt->rep,
			      &bench->opt->threads);
}
