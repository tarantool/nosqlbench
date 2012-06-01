
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
#include <inttypes.h>

#include <tarantool/tnt.h>
#include <tarantool/tnt_net.h>

#include <nb_queue.h>
#include <nb_stat.h>
#include <nb_func.h>
#include <nb_cb.h>
#include <nb_redis.h>
#include <nb_mc.h>
#include <nb_raw.h>

inline static int
nb_cb_key(char *buf, int size, int tid, int bsize, int i) {
	return snprintf(buf, size, "key_%d_%d_%d", tid, bsize, i);
}

static void nb_cb_error(struct nb_func_arg *a, char *name) {
	printf("%s failed: %s\n", name, tnt_strerror(a->t));
}

static void nb_cb_recv(struct nb_func_arg *a) {
	struct tnt_iter i;
	tnt_iter_stream(&i, a->t);
	while (tnt_next(&i)) {
		struct tnt_reply *r = TNT_ISTREAM_REPLY(&i);
		if (tnt_error(a->t) != TNT_EOK) {
			printf("error, %s\n", tnt_strerror(a->t));
		} else if (r->code != 0) {
			printf("server respond: %s (op: %"PRIu32", reqid: %"PRIu32", "
			       "code: %"PRIu32", count: %"PRIu32")\n",
				(r->error) ? r->error : "",
				r->op,
				r->reqid,
				r->code,
				r->count);
		}
	}
	if (i.status == TNT_ITER_FAIL)
		printf("recv failed: %s\n", tnt_strerror(a->t));
	tnt_iter_free(&i);
}

static void
nb_cb_insert_do(struct nb_func_arg *a, uint32_t flags) {
	char *buf = malloc(a->bsize);
	if (buf == NULL) {
		printf("memory allocation of %d bytes failed\n", a->bsize);
		return;
	}
	memset(buf, 'x', a->bsize);
	nb_stat_start(a->stat, a->count);
	int i;
	for (i = 0 ; i < a->count ; i++) {
		char key[32];
		int key_len = nb_cb_key(key, sizeof(key), a->tid, a->bsize, i);
		struct tnt_tuple tu;
		tnt_tuple_init(&tu);
		tnt_tuple_add(&tu, key, key_len);
		tnt_tuple_add(&tu, buf, a->bsize);
		if (tnt_insert(a->t, 0, flags, &tu) == -1)
			nb_cb_error(a, "insert");
		tnt_tuple_free(&tu);
	}
	tnt_flush(a->t);
	nb_cb_recv(a);
	nb_stat_stop(a->stat);
	free(buf);
}

static void nb_cb_insert(struct nb_func_arg *a) {
	nb_cb_insert_do(a, 0);
}

static void nb_cb_insert_ret(struct nb_func_arg *a) {
	nb_cb_insert_do(a, TNT_FLAG_RETURN);
}

static void nb_cb_update_do(struct nb_func_arg *a, uint32_t flags) {
	char *buf = malloc(a->bsize);
	if (buf == NULL) {
		printf("memory allocation of %d bytes failed\n", a->bsize);
		return;
	}
	memset(buf, 'x', a->bsize);
	nb_stat_start(a->stat, a->count);
	int i;
	for (i = 0 ; i < a->count ; i++) {
		char key[32];
		int key_len = nb_cb_key(key, sizeof(key), a->tid, a->bsize, i);
		struct tnt_tuple tu;
		tnt_tuple_init(&tu);
		tnt_tuple_add(&tu, key, key_len);
		struct tnt_stream u;
		tnt_buf(&u);
		tnt_update_assign(&u, 1, buf, a->bsize);
		if (tnt_update(a->t, 0, flags, &tu, &u) == -1)
			nb_cb_error(a, "update");
		tnt_stream_free(&u);
		tnt_tuple_free(&tu);
	}
	tnt_flush(a->t);
	nb_cb_recv(a);
	nb_stat_stop(a->stat);
	free(buf);
}

static void nb_cb_update(struct nb_func_arg *a) {
	nb_cb_update_do(a, 0);
}

static void nb_cb_update_ret(struct nb_func_arg *a) {
	nb_cb_update_do(a, TNT_FLAG_RETURN);
}

static void nb_cb_select(struct nb_func_arg *a) {
	nb_stat_start(a->stat, a->count);
	int i;
	for (i = 0 ; i < a->count ; i++) {
		char key[32];
		int key_len = nb_cb_key(key, sizeof(key), a->tid, a->bsize, i);
		struct tnt_tuple tu;
		tnt_tuple_init(&tu);
		tnt_tuple_add(&tu, key, key_len);
		struct tnt_list l;
		memset(&l, 0, sizeof(l));
		tnt_list_at(&l, &tu);
		if (tnt_select(a->t, 0, 0, 0, 100, &l) == -1)
			nb_cb_error(a, "select");
		tnt_list_free(&l);
		tnt_tuple_free(&tu);
	}
	tnt_flush(a->t);
	nb_cb_recv(a);
	nb_stat_stop(a->stat);
}

static void nb_cb_ping(struct nb_func_arg *a) {
	nb_stat_start(a->stat, a->count);
	int i;
	for (i = 0 ; i < a->count ; i++) {
		if (tnt_ping(a->t) == -1)
			nb_cb_error(a, "ping");
	}
	tnt_flush(a->t);
	nb_cb_recv(a);
	nb_stat_stop(a->stat);
}

static void
nb_cb_raw_insert_recv(struct nb_func_arg *a)
{
	int key;
	for (key = 0 ; key < a->count ; key++) {
		if (nb_raw_insert_recv(a->t) == -1)
			nb_cb_error(a, "recv");
		else {
			if (tnt_error(a->t) != TNT_EOK)
				printf("server respond: %s\n", tnt_strerror(a->t));
		}
	}
}

static void nb_cb_raw_insert(struct nb_func_arg *a) {
	char *buf = malloc(a->bsize);
	if (buf == NULL) {
		printf("memory allocation of %d bytes failed\n", a->bsize);
		return;
	}
	memset(buf, 'x', a->bsize);
	nb_stat_start(a->stat, a->count);
	int i;
	for (i = 0 ; i < a->count ; i++) {
		char key[32];
		nb_cb_key(key, sizeof(key), a->tid, a->bsize, i);
		if (nb_raw_insert(a->t, key, buf, a->bsize) == -1)
			nb_cb_error(a, "insert");
	}
	tnt_flush(a->t);
	nb_cb_raw_insert_recv(a);
	nb_stat_stop(a->stat);
	free(buf);
}

static void
nb_cb_redis_set_recv(struct nb_func_arg *a, int count)
{
	int key;
	for (key = 0 ; key < count ; key++) {
		if (nb_redis_set_recv(a->t) == -1)
			nb_cb_error(a, "recv");
		else {
			if (tnt_error(a->t) != TNT_EOK)
				printf("server respond: %s\n", tnt_strerror(a->t));
		}
	}
}

static void
nb_cb_redis_set(struct nb_func_arg *a)
{
	char *buf = malloc(a->bsize);
	if (buf == NULL) {
		printf("memory allocation of %d bytes failed\n", a->bsize);
		return;
	}
	memset(buf, 'x', a->bsize);
	nb_stat_start(a->stat, a->count);
	int i;
	for (i = 0 ; i < a->count ; i++) {
		char key[32];
		nb_cb_key(key, sizeof(key), a->tid, a->bsize, i);
		if (nb_redis_set(a->t, key, buf, a->bsize) == -1)
			nb_cb_error(a, "set");
	}
	tnt_flush(a->t);
	nb_cb_redis_set_recv(a, a->count);
	nb_stat_stop(a->stat);
	free(buf);
}

static void
nb_cb_redis_get_recv(struct nb_func_arg *a, int count)
{
	int key;
	for (key = 0 ; key < count ; key++) {
		char *buf = NULL;
		int buf_size;
		if (nb_redis_get_recv(a->t, &buf, &buf_size) == -1)
			nb_cb_error(a, "recv");
		else {
			if (tnt_error(a->t) != TNT_EOK)
				printf("server respond: %s\n", tnt_strerror(a->t));
		}
		if (buf)
			tnt_mem_free(buf);
	}
}

static void
nb_cb_redis_get(struct nb_func_arg *a)
{
	nb_stat_start(a->stat, a->count);
	int i;
	for (i = 0 ; i < a->count ; i++) {
		char key[32];
		nb_cb_key(key, sizeof(key), a->tid, a->bsize, i);
		if (nb_redis_get(a->t, key) == -1)
			nb_cb_error(a, "get");
	}
	tnt_flush(a->t);
	nb_cb_redis_get_recv(a, a->count);
	nb_stat_stop(a->stat);
}

static void
nb_cb_mc_set_recv(struct nb_func_arg *a, int count)
{
	int key;
	for (key = 0 ; key < count ; key++) {
		if (nb_mc_set_recv(a->t) == -1)
			nb_cb_error(a, "recv");
		else {
			if (tnt_error(a->t) != TNT_EOK)
				printf("server respond: %s\n", tnt_strerror(a->t));
		}
	}
}

static void
nb_cb_mc_set(struct nb_func_arg *a)
{
	char *buf = malloc(a->bsize);
	if (buf == NULL) {
		printf("memory allocation of %d bytes failed\n", a->bsize);
		return;
	}
	memset(buf, 'x', a->bsize);
	nb_stat_start(a->stat, a->count);
	int i;
	for (i = 0 ; i < a->count ; i++) {
		char key[32];
		nb_cb_key(key, sizeof(key), a->tid, a->bsize, i);
		if (nb_mc_set(a->t, key, buf, a->bsize) == -1)
			nb_cb_error(a, "set");
	}
	tnt_flush(a->t);
	nb_cb_mc_set_recv(a, a->count);
	nb_stat_stop(a->stat);
	free(buf);
}

static void
nb_cb_mc_get_recv(struct nb_func_arg *a, int count)
{
	int key;
	for (key = 0 ; key < count ; key++) {
		char *buf = NULL;
		int buf_size;
		if (nb_mc_get_recv(a->t, &buf, &buf_size) == -1)
			nb_cb_error(a, "recv");
		else {
			if (tnt_error(a->t) != TNT_EOK)
				printf("server respond: %s\n", tnt_strerror(a->t));
		}
		if (buf)
			tnt_mem_free(buf);
	}
}

static void
nb_cb_mc_get(struct nb_func_arg *a)
{
	nb_stat_start(a->stat, a->count);
	int i;
	for (i = 0 ; i < a->count ; i++) {
		char key[32];
		nb_cb_key(key, sizeof(key), a->tid, a->bsize, i);
		if (nb_mc_get(a->t, key) == -1)
			nb_cb_error(a, "get");
	}
	tnt_flush(a->t);
	nb_cb_mc_get_recv(a, a->count);
	nb_stat_stop(a->stat);
}

void
nb_cb_init(struct nb_funcs *funcs)
{
	nb_func_add(funcs, "tnt-insert", nb_cb_insert);
	nb_func_add(funcs, "tnt-insert-ret", nb_cb_insert_ret);
	nb_func_add(funcs, "tnt-update", nb_cb_update);
	nb_func_add(funcs, "tnt-update-ret", nb_cb_update_ret);
	nb_func_add(funcs, "tnt-select", nb_cb_select);
	nb_func_add(funcs, "tnt-ping", nb_cb_ping);

	nb_func_add(funcs, "tnt-raw-insert", nb_cb_raw_insert);

	nb_func_add(funcs, "redis-set", nb_cb_redis_set);
	nb_func_add(funcs, "redis-get", nb_cb_redis_get);

	nb_func_add(funcs, "memcache-set", nb_cb_mc_set);
	nb_func_add(funcs, "memcache-get", nb_cb_mc_get);
}
