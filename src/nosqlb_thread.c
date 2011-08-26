
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
#include <nosqlb_thread.h>

static volatile int barrier_ready;
static pthread_mutex_t barrier_up_mutex;
static pthread_mutex_t barrier_mutex;
static pthread_cond_t barrier_cond;

void
nosqlb_threads_init(struct nosqlb_threads *threads)
{
	pthread_mutex_init(&barrier_up_mutex, NULL);
	pthread_mutex_init(&barrier_mutex, NULL);
	pthread_cond_init(&barrier_cond, NULL);
	barrier_ready = 0;
	threads->count = 0;
	threads->threads = NULL;
}

void
nosqlb_threads_free(struct nosqlb_threads *threads)
{
	if (threads->threads) {
		free(threads->threads);
		threads->threads = NULL;
	}
}

void
nosqlb_threads_barrier_up(void)
{
	barrier_ready = 0;
	pthread_mutex_lock(&barrier_up_mutex);
}

void
nosqlb_threads_barrier_down(void)
{
	pthread_mutex_unlock(&barrier_up_mutex);
}

void
nosqlb_threads_barrier(struct nosqlb_threads *threads)
{
	pthread_mutex_lock(&barrier_mutex);
	while (threads->count < barrier_ready)
		pthread_cond_wait(&barrier_cond, &barrier_mutex);
	pthread_mutex_unlock(&barrier_mutex);
}

void
nosqlb_threads_barrier_ready(void)
{
	pthread_mutex_lock(&barrier_mutex);
	barrier_ready++;
	pthread_cond_signal(&barrier_cond);
	pthread_mutex_unlock(&barrier_mutex);
	/* awaiting barrier to go down */
	pthread_mutex_lock(&barrier_up_mutex);
	pthread_mutex_unlock(&barrier_up_mutex);
}

int
nosqlb_threads_create(struct nosqlb_threads *threads, int count,
	              struct nosqlb *b,
		      nosqlb_threadf_t cb,
		      struct nosqlb_test *test, struct nosqlb_test_buf *buf)
{
	threads->count = count;
	threads->threads = malloc(sizeof(struct nosqlb_thread) * count);
	if (threads->threads == NULL)
		return -1;
	memset(threads->threads, 0, sizeof(threads->threads));

	int i;
	for (i = 0 ; i < count ; i++) {
		struct nosqlb_thread *t = &threads->threads[i];
		t->idx = i;
		t->nosqlb = b;
		t->test = test;
		t->buf = buf;
		if (pthread_create(&t->thread, NULL, cb, (void*)t) == -1)
			return -1;
	}

	return 0;
}

int
nosqlb_threads_join(struct nosqlb_threads *threads)
{
	int i;
	for (i = 0 ; i < threads->count ; i++) {
		void *ret = NULL;
		pthread_join(threads->threads[i].thread, &ret);
	}
	return 0;
}
