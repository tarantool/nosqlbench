
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
#include <nb_cb.h>
#include <nb_redis.h>
#include <nb_raw.h>

static void
nb_cb_error(struct tnt *t, char *name)
{
	printf("%s failed: %s\n", name, tnt_strerror(t));
}

static void
nb_cb_recv(struct tnt *t, int count)
{
	int key;
	for (key = 0 ; key < count ; key++) {
		struct tnt_recv rcv; 
		tnt_recv_init(&rcv);
		if (tnt_recv(t, &rcv) == -1)
			nb_cb_error(t, "recv");
		else {
			if (tnt_error(t) != TNT_EOK)
				printf("server respond: %s (op: %d, reqid: %d, code: %d, count: %d)\n",
					tnt_strerror(t), TNT_RECV_OP(&rcv),
					TNT_RECV_ID(&rcv),
					TNT_RECV_CODE(&rcv),
					TNT_RECV_COUNT(&rcv));
		}
		tnt_recv_free(&rcv);
	}
}

static void
nb_cb_insert_do(struct tnt *t, int tid, char *name, int bsize, int count, int flags,
		struct nb_stat *stat)
{
	char *buf = malloc(bsize);
	if (buf == NULL) {
		printf("memory allocation of %d bytes failed\n", bsize);
		return;
	}
	memset(buf, 'x', bsize);

	nb_stat_start(stat, count);

	int i;
	for (i = 0 ; i < count ; i++) {
		char key[32];
		int key_len = snprintf(key, sizeof(key), "key_%d_%d_%d", tid, bsize, i);
		struct tnt_tuple tu;
		tnt_tuple_init(&tu);
		tnt_tuple_add(&tu, key, key_len);
		tnt_tuple_add(&tu, buf, bsize);
		if (tnt_insert(t, i, 0, flags, &tu) == -1)
			nb_cb_error(t, name);
		tnt_tuple_free(&tu);
	}

	tnt_flush(t);
	nb_cb_recv(t, count);
	nb_stat_stop(stat);

	free(buf);
}

static void
nb_cb_insert_do_sync(struct tnt *t, int tid, char *name, int bsize, int count,
		     int flags, struct nb_stat *stat)
{
	char *buf = malloc(bsize);
	if (buf == NULL) {
		printf("memory allocation of %d bytes failed\n", bsize);
		return;
	}
	memset(buf, 'x', bsize);

	nb_stat_start(stat, count);

	int i;
	for (i = 0 ; i < count ; i++) {
		char key[32];
		int key_len = snprintf(key, sizeof(key), "key_%d_%d_%d", tid, bsize, i);
		struct tnt_tuple tu;
		tnt_tuple_init(&tu);
		tnt_tuple_add(&tu, key, key_len);
		tnt_tuple_add(&tu, buf, bsize);
		if (tnt_insert(t, i, 0, flags, &tu) == -1)
			nb_cb_error(t, name);
		tnt_flush(t);
		tnt_tuple_free(&tu);

		nb_cb_recv(t, 1);
	}

	nb_stat_stop(stat);
	free(buf);
}

static void
nb_cb_insert(struct tnt *t, int tid, int bsize, int count,
             struct nb_stat *stat)
{
	nb_cb_insert_do(t, tid, "insert", bsize, count, 0, stat);
}

static void
nb_cb_insert_ret(struct tnt *t, int tid, int bsize, int count,
		 struct nb_stat *stat)
{
	nb_cb_insert_do(t, tid, "insert-ret",
		bsize, count, TNT_PROTO_FLAG_RETURN, stat);
}

static void
nb_cb_insert_sync(struct tnt *t, int tid, int bsize, int count,
		  struct nb_stat *stat)
{
	nb_cb_insert_do_sync(t, tid, "sync-insert",
		bsize, count, 0, stat);
}

static void
nb_cb_insert_ret_sync(struct tnt *t, int tid, int bsize, int count,
		      struct nb_stat *stat)
{
	nb_cb_insert_do_sync(t, tid, "sync-insert-ret",
		bsize, count, TNT_PROTO_FLAG_RETURN, stat);
}

static void
nb_cb_update_do(struct tnt *t, int tid, char *name, int bsize, int count,
		int flags, struct nb_stat *stat)
{
	char *buf = malloc(bsize);
	if (buf == NULL) {
		printf("memory allocation of %d bytes failed\n", bsize);
		return;
	}
	memset(buf, 'x', bsize);

	nb_stat_start(stat, count);

	int i;
	for (i = 0 ; i < count ; i++) {
		char key[32];
		int key_len = snprintf(key, sizeof(key), "key_%d_%d_%d", tid, bsize, i);
		struct tnt_update u;
		tnt_update_init(&u);
		tnt_update_assign(&u, 1, buf, bsize);
		if (tnt_update(t, i, 0, flags, key, key_len, &u) == -1)
			nb_cb_error(t, name);
		tnt_update_free(&u);
	}

	tnt_flush(t);
	nb_cb_recv(t, count);
	nb_stat_stop(stat);

	free(buf);
}
static void
nb_cb_update_do_sync(struct tnt *t, int tid, char *name, int bsize, int count, int flags,
		     struct nb_stat *stat)
{
	char *buf = malloc(bsize);
	if (buf == NULL) {
		printf("memory allocation of %d bytes failed\n", bsize);
		return;
	}
	memset(buf, 'x', bsize);

	nb_stat_start(stat, count);

	int i;
	for (i = 0 ; i < count ; i++) {
		char key[32];
		int key_len = snprintf(key, sizeof(key), "key_%d_%d_%d", tid, bsize, i);
		struct tnt_update u;
		tnt_update_init(&u);
		tnt_update_assign(&u, 1, buf, bsize);
		if (tnt_update(t, i, 0, flags, key, key_len, &u) == -1)
			nb_cb_error(t, name);
		tnt_flush(t);
		tnt_update_free(&u);

		nb_cb_recv(t, 1);
	}

	nb_stat_stop(stat);
	free(buf);
}

static void
nb_cb_update(struct tnt *t, int tid, int bsize, int count,
	     struct nb_stat *stat)
{
	nb_cb_update_do(t, tid, "update", bsize, count, 0, stat);
}

static void
nb_cb_update_ret(struct tnt *t, int tid, int bsize, int count,
		 struct nb_stat *stat)
{
	nb_cb_update_do(t, tid, "update-ret",
		bsize, count, TNT_PROTO_FLAG_RETURN, stat);
}

static void
nb_cb_update_sync(struct tnt *t, int tid, int bsize, int count,
		  struct nb_stat *stat)
{
	nb_cb_update_do_sync(t, tid, "sync-update",
		bsize, count, 0, stat);
}

static void
nb_cb_update_ret_sync(struct tnt *t, int tid, int bsize, int count,
		      struct nb_stat *stat)
{
	nb_cb_update_do_sync(t, tid, "sync-update-ret",
		bsize, count, TNT_PROTO_FLAG_RETURN, stat);
}

static void
nb_cb_select(struct tnt *t, int tid, int bsize, int count,
	     struct nb_stat *stat)
{
	nb_stat_start(stat, count);

	int i;
	for (i = 0 ; i < count ; i++) {
		char key[32];
		int key_len = snprintf(key, sizeof(key), "key_%d_%d_%d", tid, bsize, i);
		struct tnt_tuples tuples;
		tnt_tuples_init(&tuples);
		struct tnt_tuple * tu = tnt_tuples_add(&tuples);
		tnt_tuple_init(tu);
		tnt_tuple_add(tu, key, key_len);
		if (tnt_select(t, i, 0, 0, 0, 100, &tuples) == -1)
			nb_cb_error(t, "select");
		tnt_tuples_free(&tuples);
	}

	tnt_flush(t);
	nb_cb_recv(t, count);
	nb_stat_stop(stat);
}

static void
nb_cb_select_sync(struct tnt *t, int tid, int bsize, int count,
		  struct nb_stat *stat)
{
	nb_stat_start(stat, count);

	int i;
	for (i = 0 ; i < count ; i++) {
		char key[32];
		int key_len = snprintf(key, sizeof(key), "key_%d_%d_%d", tid, bsize, i);
		struct tnt_tuples tuples;
		tnt_tuples_init(&tuples);
		struct tnt_tuple * tu = tnt_tuples_add(&tuples);
		tnt_tuple_init(tu);
		tnt_tuple_add(tu, key, key_len);
		if (tnt_select(t, i, 0, 0, 0, 100, &tuples) == -1)
			nb_cb_error(t, "sync-select");
		tnt_flush(t);
		tnt_tuples_free(&tuples);

		nb_cb_recv(t, 1);
	}

	nb_stat_stop(stat);
}

static void
nb_cb_ping(struct tnt *t, int tid __attribute__((unused)),
	   int bsize __attribute__((unused)), int count,
           struct nb_stat *stat)
{
	nb_stat_start(stat, count);

	int i;
	for (i = 0 ; i < count ; i++) {
		if (tnt_ping(t, i) == -1)
			nb_cb_error(t, "ping");
	}

	tnt_flush(t);
	nb_cb_recv(t, count);
	nb_stat_stop(stat);
}

static void
nb_cb_ping_sync(struct tnt *t, int tid __attribute__((unused)),
		int bsize __attribute__((unused)), int count,
		struct nb_stat *stat)
{
	nb_stat_start(stat, count);

	int i;
	for (i = 0 ; i < count ; i++) {
		if (tnt_ping(t, i) == -1)
			nb_cb_error(t, "sync-ping");
		tnt_flush(t);
		nb_cb_recv(t, 1);
	}

	nb_stat_stop(stat);
}

static void
nb_cb_memcache_set(struct tnt *t, int tid, int bsize, int count,
		   struct nb_stat *stat)
{
	char *buf = malloc(bsize);
	if (buf == NULL) {
		printf("memory allocation of %d bytes failed\n", bsize);
		return;
	}
	memset(buf, 'x', bsize);

	nb_stat_start(stat, count);

	int i;
	for (i = 0 ; i < count ; i++) {
		char key[32];
		snprintf(key, sizeof(key), "key_%d_%d_%d", tid, bsize, i);
		if (tnt_memcache_set(t, 0, 0, key, buf, bsize) == -1)
			nb_cb_error(t, "set");
	}

	nb_stat_stop(stat);
	free(buf);
}

static void
nb_cb_memcache_get(struct tnt *t, int tid, int bsize, int count,
		   struct nb_stat *stat)
{
	nb_stat_start(stat, count);

	int key;
	char keydesc[32];
	char *keyptr[1] = { keydesc };

	for (key = 0 ; key < count ; key++) {
		snprintf(keydesc, sizeof(keydesc), "key_%d_%d_%d", tid, bsize, key);

		struct tnt_memcache_vals vals;
		tnt_memcache_val_init(&vals);

		if (tnt_memcache_get(t, 0, 1, keyptr, &vals) == -1)
			nb_cb_error(t, "get");

		tnt_memcache_val_free(&vals);
	}

	nb_stat_stop(stat);
}

static void
nb_cb_redis_set_recv(struct tnt *t, int count)
{
	int key;
	for (key = 0 ; key < count ; key++) {
		if (nb_redis_set_recv(t) == -1)
			nb_cb_error(t, "recv");
		else {
			if (tnt_error(t) != TNT_EOK)
				printf("server respond: %s\n", tnt_strerror(t));
		}
	}
}

static void
nb_cb_redis_set(struct tnt *t, int tid, int bsize, int count,
		    struct nb_stat *stat)
{
	char *buf = malloc(bsize);
	if (buf == NULL) {
		printf("memory allocation of %d bytes failed\n", bsize);
		return;
	}
	memset(buf, 'x', bsize);

	nb_stat_start(stat, count);

	int i;
	for (i = 0 ; i < count ; i++) {
		char key[32];
		snprintf(key, sizeof(key), "key_%d_%d_%d", tid, bsize, i);
		if (nb_redis_set(t, key, buf, bsize) == -1)
			nb_cb_error(t, "set");
	}

	tnt_flush(t);
	nb_cb_redis_set_recv(t, count);
	nb_stat_stop(stat);

	free(buf);
}

static void
nb_cb_redis_set_sync(struct tnt *t, int tid, int bsize, int count,
		     struct nb_stat *stat)
{
	char *buf = malloc(bsize);
	if (buf == NULL) {
		printf("memory allocation of %d bytes failed\n", bsize);
		return;
	}
	memset(buf, 'x', bsize);

	nb_stat_start(stat, count);

	int i;
	for (i = 0 ; i < count ; i++) {
		char key[32];
		snprintf(key, sizeof(key), "key_%d_%d_%d", tid, bsize, i);
		if (nb_redis_set(t, key, buf, bsize) == -1)
			nb_cb_error(t, "set");
		tnt_flush(t);
		nb_cb_redis_set_recv(t, 1);
	}

	nb_stat_stop(stat);
	free(buf);
}

static void
nb_cb_redis_get_recv(struct tnt *t, int count)
{
	int key;
	for (key = 0 ; key < count ; key++) {
		char *buf;
		int buf_size;
		if (nb_redis_get_recv(t, &buf, &buf_size) == -1)
			nb_cb_error(t, "recv");
		else {
			if (tnt_error(t) != TNT_EOK)
				printf("server respond: %s\n", tnt_strerror(t));
		}
		tnt_mem_free(buf);
	}
}

static void
nb_cb_redis_get(struct tnt *t, int tid, int bsize, int count,
		struct nb_stat *stat)
{
	nb_stat_start(stat, count);

	int i;
	for (i = 0 ; i < count ; i++) {
		char key[32];
		snprintf(key, sizeof(key), "key_%d_%d_%d", tid, bsize, i);
		if (nb_redis_get(t, key) == -1)
			nb_cb_error(t, "get");
	}

	tnt_flush(t);
	nb_cb_redis_get_recv(t, count);
	nb_stat_stop(stat);
}

static void
nb_cb_redis_get_sync(struct tnt *t, int tid, int bsize, int count,
		     struct nb_stat *stat)
{
	nb_stat_start(stat, count);

	int i;
	for (i = 0 ; i < count ; i++) {
		char key[32];
		snprintf(key, sizeof(key), "key_%d_%d_%d", tid, bsize, i);
		if (nb_redis_get(t, key) == -1)
			nb_cb_error(t, "get");
		tnt_flush(t);
		nb_cb_redis_get_recv(t, 1);
	}

	nb_stat_stop(stat);
}

static void
nb_cb_raw_insert_recv(struct tnt *t, int count)
{
	int key;
	for (key = 0 ; key < count ; key++) {
		if (nb_raw_insert_recv(t) == -1)
			nb_cb_error(t, "recv");
		else {
			if (tnt_error(t) != TNT_EOK)
				printf("server respond: %s\n", tnt_strerror(t));
		}
	}
}

static void
nb_cb_raw_insert(struct tnt *t, int tid, int bsize, int count,
		 struct nb_stat *stat)
{
	char *buf = malloc(bsize);
	if (buf == NULL) {
		printf("memory allocation of %d bytes failed\n", bsize);
		return;
	}
	memset(buf, 'x', bsize);

	nb_stat_start(stat, count);

	int i;
	for (i = 0 ; i < count ; i++) {
		char key[32];
		snprintf(key, sizeof(key), "key_%d_%d_%d", tid, bsize, i);
		if (nb_raw_insert(t, key, buf, bsize) == -1)
			nb_cb_error(t, "insert");
	}

	tnt_flush(t);
	nb_cb_raw_insert_recv(t, count);
	nb_stat_stop(stat);

	free(buf);
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

	nb_func_add(funcs, "tnt-insert-sync", nb_cb_insert_sync);
	nb_func_add(funcs, "tnt-insert-ret-sync", nb_cb_insert_ret_sync);
	nb_func_add(funcs, "tnt-update-sync", nb_cb_update_sync);
	nb_func_add(funcs, "tnt-update-ret-sync", nb_cb_update_ret_sync);
	nb_func_add(funcs, "tnt-select-sync", nb_cb_select_sync);
	nb_func_add(funcs, "tnt-ping-sync", nb_cb_ping_sync);

	nb_func_add(funcs, "tnt-raw-insert", nb_cb_raw_insert);

	nb_func_add(funcs, "memcache-set", nb_cb_memcache_set);
	nb_func_add(funcs, "memcache-get", nb_cb_memcache_get);

	nb_func_add(funcs, "redis-set", nb_cb_redis_set);
	nb_func_add(funcs, "redis-get", nb_cb_redis_get);
	nb_func_add(funcs, "redis-set-sync", nb_cb_redis_set_sync);
	nb_func_add(funcs, "redis-get-sync", nb_cb_redis_get_sync);
}
