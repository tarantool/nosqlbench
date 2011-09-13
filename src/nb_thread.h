#ifndef NB_THREAD_H_INCLUDED
#define NB_THREAD_H_INCLUDED

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

typedef void *(*nb_threadf_t)(void*);

struct nb;
struct nb_test;
struct nb_test_buf;

struct nb_thread {
	int idx;
	pthread_t thread;
	struct nb *nb;
	struct nb_test *test;
	struct nb_test_buf *buf;
	struct nb_stat stat;
};

struct nb_threads {
	int count;
	struct nb_thread *threads;
};

void nb_threads_init(struct nb_threads *threads);
void nb_threads_free(struct nb_threads *threads);

int nb_threads_create(struct nb_threads *threads, int count,
		      struct nb *b,
		      nb_threadf_t cb);

int nb_threads_join(struct nb_threads *threads);

void nb_threads_set(struct nb_threads *threads,
		    struct nb_test *test, struct nb_test_buf *buf);

#endif /* NB_WORKER_H_INCLUDED */
