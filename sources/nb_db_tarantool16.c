
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

#define MP_SOURCE 1
#include <msgpuck.h>

#include <lib/tarantool.h>
#include <lib/iproto.h>

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
	int await;
};

static inline void
db_tarantool16_inc(struct nb_db *db)
{
	struct db_tarantool16 *t = db->priv;
	t->await++;
}

static inline void
db_tarantool16_dec(struct nb_db *db)
{
	struct db_tarantool16 *t = db->priv;
	t->await--;
}

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
	char *p = t->buf + 5;
	p = mp_encode_map(p, 1);
	p = mp_encode_uint(p, TB_CODE);
	p = mp_encode_uint(p, TB_INSERT);
	p = mp_encode_map(p, 2);
	p = mp_encode_uint(p, TB_SPACE);
	p = mp_encode_uint(p, 0);
	p = mp_encode_uint(p, TB_TUPLE);
	p = mp_encode_array(p, 2);
	switch (key->size) {
	case 4: p = mp_encode_uint(p, *(uint32_t*)key->data);
		break;
	case 8: p = mp_encode_uint(p, *(uint64_t*)key->data);
		break;
	default:
		p = mp_encode_str(p, key->data, key->size);
		break;
	}
	p = mp_encode_str(p, t->value, t->value_size);
	uint32_t size = p - t->buf;
	assert(size <= t->buf_size);
	*t->buf = 0xce;
	*(uint32_t*)(t->buf+1) = mp_bswap_u32(size - 5);
	int rc = tb_sessend(&t->s, t->buf, size);
	if (rc == -1)
		return -1;
	db_tarantool16_inc(db);
	return 0;
}

static int db_tarantool16_replace(struct nb_db *db, struct nb_key *key)
{
	struct db_tarantool16 *t = db->priv;
	char *p = t->buf + 5;
	p = mp_encode_map(p, 1);
	p = mp_encode_uint(p, TB_CODE);
	p = mp_encode_uint(p, TB_REPLACE);
	p = mp_encode_map(p, 2);
	p = mp_encode_uint(p, TB_SPACE);
	p = mp_encode_uint(p, 0);
	p = mp_encode_uint(p, TB_TUPLE);
	p = mp_encode_array(p, 2);
	switch (key->size) {
	case 4: p = mp_encode_uint(p, *(uint32_t*)key->data);
		break;
	case 8: p = mp_encode_uint(p, *(uint64_t*)key->data);
		break;
	default:
		p = mp_encode_str(p, key->data, key->size);
		break;
	}
	p = mp_encode_str(p, t->value, t->value_size);
	uint32_t size = p - t->buf;
	assert(size <= t->buf_size);
	*t->buf = 0xce;
	*(uint32_t*)(t->buf+1) = mp_bswap_u32(size - 5);
	int rc = tb_sessend(&t->s, t->buf, size);
	if (rc == -1)
		return -1;
	db_tarantool16_inc(db);
	return 0;
}

static int db_tarantool16_delete(struct nb_db *db, struct nb_key *key)
{
	struct db_tarantool16 *t = db->priv;
	char *p = t->buf + 5;
	p = mp_encode_map(p, 1);
	p = mp_encode_uint(p, TB_CODE);
	p = mp_encode_uint(p, TB_REPLACE);
	p = mp_encode_map(p, 2);
	p = mp_encode_uint(p, TB_SPACE);
	p = mp_encode_uint(p, 0);
	p = mp_encode_uint(p, TB_KEY);
	p = mp_encode_array(p, 1);
	switch (key->size) {
	case 4: p = mp_encode_uint(p, *(uint32_t*)key->data);
		break;
	case 8: p = mp_encode_uint(p, *(uint64_t*)key->data);
		break;
	default:
		p = mp_encode_str(p, key->data, key->size);
		break;
	}
	uint32_t size = p - t->buf;
	assert(size <= t->buf_size);
	*t->buf = 0xce;
	*(uint32_t*)(t->buf+1) = mp_bswap_u32(size - 5);
	int rc = tb_sessend(&t->s, t->buf, size);
	if (rc == -1)
		return -1;
	db_tarantool16_inc(db);
	return 0;
}

static int db_tarantool16_update(struct nb_db *db, struct nb_key *key)
{
	struct db_tarantool16 *t = db->priv;
	char *p = t->buf + 5;
	p = mp_encode_map(p, 1);
	p = mp_encode_uint(p, TB_CODE);
	p = mp_encode_uint(p, TB_UPDATE);
	p = mp_encode_map(p, 3);
	p = mp_encode_uint(p, TB_SPACE);
	p = mp_encode_uint(p, 0);
	p = mp_encode_uint(p, TB_KEY);
	p = mp_encode_array(p, 1);
	switch (key->size) {
	case 4: p = mp_encode_uint(p, *(uint32_t*)key->data);
		break;
	case 8: p = mp_encode_uint(p, *(uint64_t*)key->data);
		break;
	default:
		p = mp_encode_str(p, key->data, key->size);
		break;
	}
	p = mp_encode_uint(p, TB_TUPLE);
	p = mp_encode_array(p, 1);
	p = mp_encode_array(p, 3);
	p = mp_encode_str(p, "=", 1);
	p = mp_encode_uint(p, 1);
	p = mp_encode_str(p, t->value, t->value_size);
	uint32_t size = p - t->buf;
	assert(size <= t->buf_size);
	*t->buf = 0xce;
	*(uint32_t*)(t->buf+1) = mp_bswap_u32(size - 5);
	int rc = tb_sessend(&t->s, t->buf, size);
	if (rc == -1)
		return -1;
	db_tarantool16_inc(db);
	return 0;
}

static int db_tarantool16_select(struct nb_db *db, struct nb_key *key)
{
	struct db_tarantool16 *t = db->priv;
	char *p = t->buf + 5;
	p = mp_encode_map(p, 1);
	p = mp_encode_uint(p, TB_CODE);
	p = mp_encode_uint(p, TB_SELECT);
	p = mp_encode_map(p, 3);
	p = mp_encode_uint(p, TB_SPACE);
	p = mp_encode_uint(p, 0);
	p = mp_encode_uint(p, TB_INDEX);
	p = mp_encode_uint(p, 0);
	p = mp_encode_uint(p, TB_KEY);
	p = mp_encode_array(p, 1);
	switch (key->size) {
	case 4: p = mp_encode_uint(p, *(uint32_t*)key->data);
		break;
	case 8: p = mp_encode_uint(p, *(uint64_t*)key->data);
		break;
	default:
		p = mp_encode_str(p, key->data, key->size);
		break;
	}
	uint32_t size = p - t->buf;
	assert(size <= t->buf_size);
	*t->buf = 0xce;
	*(uint32_t*)(t->buf+1) = mp_bswap_u32(size - 5);
	int rc = tb_sessend(&t->s, t->buf, size);
	if (rc == -1)
		return -1;
	db_tarantool16_inc(db);
	return 0;
}

static int db_tarantool16_recv(struct nb_db *db, int count, int *missed)
{
	(void)missed;
	struct db_tarantool16 *t = db->priv;
	int rc = tb_sessync(&t->s);
	if (rc == -1)
		return -1;
	while (t->await > 0 && count >= 0)
	{
		ssize_t len = tb_sesrecv(&t->s, t->buf, 5, 5);
		if (len == -1) {
			printf("recv failed\n");
			return 0;
		}
		if (mp_typeof(*t->buf) != MP_UINT) {
			printf("bady reply len type\n");
			return 0;
		}
		const char *p = t->buf;
		uint32_t body = mp_decode_uint(&p);
		if (5 + body >= t->buf_size) {
			printf("reply buffer is too big\n");
			return -1;
		}
		len = tb_sesrecv(&t->s, t->buf + 5, body, body);
		if (len == -1) {
			printf("recv failed\n");
			return 0;
		}
		struct tbresponse rp;
		int64_t r = tb_response(&rp, t->buf, 5 + body);
		if (r == -1) {
			printf("failed to parse response\n");
			return 0;
		}
		if (rp.code != 0)
			printf("server respond: %d, %-.*s\n", rp.code,
			       (int)(rp.error_end - rp.error), rp.error);
		t->await--;
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
