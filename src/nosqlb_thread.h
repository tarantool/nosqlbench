#ifndef NOSQLB_THREAD_H_INCLUDED
#define NOSQLB_THREAD_H_INCLUDED

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

typedef void *(*nosqlb_threadf_t)(void*);

struct nosqlb;
struct nosqlb_test;
struct nosqlb_test_buf;

struct nosqlb_thread {
	int idx;
	pthread_t thread;
	struct nosqlb *nosqlb;
	struct nosqlb_test *test;
	struct nosqlb_test_buf *buf;
	struct nosqlb_stat stat;
};

struct nosqlb_threads {
	int count;
	struct nosqlb_thread *threads;
};

void nosqlb_threads_init(struct nosqlb_threads *threads);
void nosqlb_threads_free(struct nosqlb_threads *threads);

int nosqlb_threads_create(struct nosqlb_threads *threads, int count,
		          struct nosqlb *b,
			  nosqlb_threadf_t cb);

int nosqlb_threads_join(struct nosqlb_threads *threads);

void nosqlb_threads_set(struct nosqlb_threads *threads,
		        struct nosqlb_test *test, struct nosqlb_test_buf *buf);

#endif /* NOSQLB_WORKER_H_INCLUDED */
