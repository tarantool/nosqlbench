
/*
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>

#include "src/tp.h"
#include "src/session.h"

#include "nb_alloc.h"
#include "nb_opt.h"
#include "nb_key.h"
#include "nb_db.h"
#include "nb_db_tarantool16.h"

struct db_tarantool16 {
	struct tbses s;
	char *value;
	size_t value_size;
	char *buf;
	size_t buf_size;
};

static int db_tarantool16_init(struct nb_db *db, size_t value_size) {
	db->priv = nb_malloc(sizeof(struct db_tarantool16));
	memset(db->priv, 0, sizeof(struct db_tarantool16));
	struct db_tarantool16 *t = db->priv;
	tb_sesinit(&t->s);
	t->value_size = value_size;
	t->value = nb_malloc(t->value_size);
	memset(t->value, '#', t->value_size - 1);
	t->value[t->value_size - 1] = 0;
	t->buf_size = 1024 + value_size;
	t->buf = nb_malloc(t->buf_size);
	return 0;
}

static void db_tarantool16_free(struct nb_db *db)
{
	struct db_tarantool16 *t = db->priv;
	tb_sesclose(&t->s);
	tb_sesfree(&t->s);
	if (t->value)
		free(t->value);
	if (t->buf)
		free(t->buf);
	free(db->priv);
	db->priv = NULL;
}

static int
db_tarantool16_connect(struct nb_db *db, struct nb_options *opts)
{
	struct db_tarantool16 *t = db->priv;
	tb_sesset(&t->s, TB_HOST, opts->host);
	tb_sesset(&t->s, TB_PORT, opts->port);
	tb_sesset(&t->s, TB_SENDBUF, opts->buf_send);
	tb_sesset(&t->s, TB_READBUF, opts->buf_recv);
	int rc = tb_sesconnect(&t->s);
	if (rc == -1) {
		printf("tnt_connect() failed: %d\n", t->s.errno_);
		return -1;
	}
	char greet[128];
	tb_sesrecv(&t->s, greet, 128, 1);
	return 0;
}

static void db_tarantool16_close(struct nb_db *db)
{
	struct db_tarantool16 *t = db->priv;
	tb_sesclose(&t->s);
}

static int db_tarantool16_insert(struct nb_db *db, struct nb_key *key)
{
	struct db_tarantool16 *t = db->priv;

	struct tp req;
	tp_init(&req, t->buf, t->buf_size, NULL, NULL);
	tp_insert(&req, 512);
	tp_tuple(&req, 2);
	switch (key->size) {
	case 4:
		tp_encode_uint(&req, *(uint32_t*)key->data);
		break;
	case 8:
		tp_encode_uint(&req, *(uint64_t*)key->data);
		break;
	default:
		tp_encode_str(&req, key->data, key->size);
		break;
	}
	tp_encode_str(&req, t->value, t->value_size);
	uint32_t size = tp_used(&req);
	assert(size <= t->buf_size);
	int rc = tb_sessend(&t->s, t->buf, size);
	if (rc == -1)
		return -1;
	return 0;
}

static int db_tarantool16_replace(struct nb_db *db, struct nb_key *key)
{
	struct db_tarantool16 *t = db->priv;

	struct tp req;
	tp_init(&req, t->buf, t->buf_size, NULL, NULL);
	tp_replace(&req, 512);
	tp_tuple(&req, 2);
	switch (key->size) {
	case 4:
		tp_encode_uint(&req, *(uint32_t*)key->data);
		break;
	case 8:
		tp_encode_uint(&req, *(uint64_t*)key->data);
		break;
	default:
		tp_encode_str(&req, key->data, key->size);
		break;
	}
	tp_encode_str(&req, t->value, t->value_size);
	uint32_t size = tp_used(&req);
	assert(size <= t->buf_size);
	int rc = tb_sessend(&t->s, t->buf, size);
	if (rc == -1)
		return -1;
	return 0;
}

static int db_tarantool16_delete(struct nb_db *db, struct nb_key *key)
{
	struct db_tarantool16 *t = db->priv;

	struct tp req;
	tp_init(&req, t->buf, t->buf_size, NULL, NULL);
	tp_delete(&req, 512);
	tp_key(&req, 1);
	switch (key->size) {
	case 4:
		tp_encode_uint(&req, *(uint32_t*)key->data);
		break;
	case 8:
		tp_encode_uint(&req, *(uint64_t*)key->data);
		break;
	default:
		tp_encode_str(&req, key->data, key->size);
		break;
	}
	uint32_t size = tp_used(&req);
	assert(size <= t->buf_size);
	int rc = tb_sessend(&t->s, t->buf, size);
	if (rc == -1)
		return -1;
	return 0;
}

static int db_tarantool16_update(struct nb_db *db, struct nb_key *key)
{
	struct db_tarantool16 *t = db->priv;

	struct tp req;
	tp_init(&req, t->buf, t->buf_size, NULL, NULL);
	tp_update(&req, 512);
	tp_key(&req, 1);
	switch (key->size) {
	case 4:
		tp_encode_uint(&req, *(uint32_t*)key->data);
		break;
	case 8:
		tp_encode_uint(&req, *(uint64_t*)key->data);
		break;
	default:
		tp_encode_str(&req, key->data, key->size);
		break;
	}
	tp_updatebegin(&req, 1);
	tp_op(&req, '=', 2);
	tp_encode_str(&req, t->value, t->value_size);
	uint32_t size = tp_used(&req);
	assert(size <= t->buf_size);
	int rc = tb_sessend(&t->s, t->buf, size);
	if (rc == -1)
		return -1;
	return 0;
}

static int db_tarantool16_select(struct nb_db *db, struct nb_key *key)
{
	struct db_tarantool16 *t = db->priv;

	struct tp req;
	tp_init(&req, t->buf, t->buf_size, NULL, NULL);
	tp_select(&req, 512, 0, 0, 1);
	tp_key(&req, 1);
	switch (key->size) {
	case 4:
		tp_encode_uint(&req, *(uint32_t*)key->data);
		break;
	case 8:
		tp_encode_uint(&req, *(uint64_t*)key->data);
		break;
	default:
		tp_encode_str(&req, key->data, key->size);
		break;
	}
	uint32_t size = tp_used(&req);
	assert(size <= t->buf_size);
	int rc = tb_sessend(&t->s, t->buf, size);
	if (rc == -1)
		return -1;
	return 0;
}

static int db_tarantool16_recv(struct nb_db *db, int count, int *missed)
{
	(void)missed;
	struct db_tarantool16 *t = db->priv;
	int rc = tb_sessync(&t->s);
	if (rc == -1) {
		printf("sync failed\n");
		return -1;
	}
	while (count > 0)
	{
		ssize_t len = tb_sesrecv(&t->s, t->buf, 5, 1);
		if (len < 5) {
			printf("recv failed\n");
			return 0;
		}
		if (mp_typeof(*t->buf) != MP_UINT) {
			printf("bad reply len type\n");
			return 0;
		}
		const char *p = t->buf;
		uint32_t body = mp_decode_uint(&p);
		if (5 + body >= t->buf_size) {
			printf("reply buffer is too big\n");
			return -1;
		}
		len = tb_sesrecv(&t->s, t->buf + 5, body, 1);
		if (len < body) {
			printf("recv failed\n");
			return -1;
		}
		struct tpresponse rp;
		int64_t r = tp_reply(&rp, t->buf, 5 + body);
		if (r == -1) {
			printf("failed to parse response\n");
			return -1;
		}
		if (rp.code != 0)
			printf("server respond: %d, %-.*s\n", rp.code,
			       (int)(rp.error_end - rp.error), rp.error);
		count--;
	}

	return 0;
}

struct nb_db_if nb_db_tarantool16 =
{
	.name    = "tarantool1_6",
	.init    = db_tarantool16_init,
	.free    = db_tarantool16_free,
	.connect = db_tarantool16_connect,
	.close   = db_tarantool16_close,
	.insert  = db_tarantool16_insert,
	.replace = db_tarantool16_replace,
	.del     = db_tarantool16_delete,
	.update  = db_tarantool16_update,
	.select  = db_tarantool16_select,
	.recv    = db_tarantool16_recv
};
