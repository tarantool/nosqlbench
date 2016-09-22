
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

#include <tarantool/tarantool.h>
#include <tarantool/tnt_net.h>
#include <tarantool/tnt_opt.h>

#include "msgpuck/msgpuck.h"

#include "nb_alloc.h"
#include "nb.h"
#include "nb_key.h"
#include "nb_db.h"
#include "nb_db_tarantool16.h"

extern struct nb nb;

struct db_tarantool16 {
	struct tnt_stream *stream;
	struct tnt_stream *object;
	struct tnt_stream *update_buf;
	char *value;
	size_t value_size;
};

static int db_tarantool16_init(struct nb_db *db, size_t value_size) {
	db->priv = nb_malloc(sizeof(struct db_tarantool16));
	struct db_tarantool16 *t = db->priv;
	memset(db->priv, 0, sizeof(struct db_tarantool16));
	t->stream = tnt_net(NULL);
	nb_oom(t->stream);
	t->object = tnt_object(NULL);
	nb_oom(t->object);
	t->update_buf = tnt_update_container(NULL);
	nb_oom(t->update_buf);
	t->value_size = value_size;
	t->value = nb_malloc(t->value_size);
	memset(t->value, '#', t->value_size - 1);
	t->value[t->value_size - 1] = 0;
	return 0;
}

static void db_tarantool16_free(struct nb_db *db)
{
	struct db_tarantool16 *t = db->priv;
	tnt_close(t->stream);
	tnt_stream_free(t->stream);
	tnt_stream_free(t->object);
	free(t->value);
	free(t);
	db->priv = NULL;
}

static int
db_tarantool16_connect(struct nb_db *db, struct nb_options *opts)
{
	struct db_tarantool16 *t = db->priv;
	char uri[256]; memset(uri, 0, 256);
	snprintf(uri, 256, "%s:%d", opts->host, opts->port);
	tnt_set(t->stream, TNT_OPT_URI, uri);
	tnt_set(t->stream, TNT_OPT_SEND_BUF, opts->buf_send);
	tnt_set(t->stream, TNT_OPT_RECV_BUF, opts->buf_recv);
	int rc = tnt_connect(t->stream);
	if (rc == -1) {
		printf("tnt_connect() failed: %s\n", tnt_strerror(t->stream));
		return -1;
	}
	return 0;
}

static void db_tarantool16_close(struct nb_db *db)
{
	struct db_tarantool16 *t = db->priv;
	tnt_close(t->stream);
}

static inline int
encode_key(struct tnt_stream *obj, struct nb_key *key)
{
	switch (key->size) {
	case 4:
		tnt_object_add_int(obj, *(uint32_t*)key->data);
		break;
	case 8:
		tnt_object_add_int(obj, *(uint64_t*)key->data);
		break;
	default:
		tnt_object_add_str(obj, key->data, key->size);
		break;
	}
	return 0;
}

void *db_tarantool16_get_buf(struct nb_db *db, size_t *size)
{
	struct db_tarantool16 *t = db->priv;
	struct tnt_stream_net *sn = TNT_SNET_CAST(t->stream);
	*size = sn->sbuf.off;
	sn->sbuf.off = 0;
	return sn->sbuf.buf;
}

static int db_tarantool16_insert(struct nb_db *db, struct nb_key *key)
{
	struct db_tarantool16 *t = db->priv;

	tnt_object_reset(t->object);
	tnt_object_add_array(t->object, 2);
	encode_key(t->object, key);
	tnt_object_add_str(t->object, t->value, t->value_size);
	t->stream->reqid = nb.opts.get_time();

	return tnt_insert(t->stream, 512, t->object);
}

static int db_tarantool16_replace(struct nb_db *db, struct nb_key *key)
{
	struct db_tarantool16 *t = db->priv;

	tnt_object_reset(t->object);
	tnt_object_add_array(t->object, 2);
	encode_key(t->object, key);
	tnt_object_add_str(t->object, t->value, t->value_size);
	t->stream->reqid = nb.opts.get_time();

	return tnt_replace(t->stream, 512, t->object);
}

static int db_tarantool16_delete(struct nb_db *db, struct nb_key *key)
{
	struct db_tarantool16 *t = db->priv;

	tnt_object_reset(t->object);
	tnt_object_add_array(t->object, 1);
	encode_key(t->object, key);
	t->stream->reqid = nb.opts.get_time();

	return tnt_delete(t->stream, 512, 0, t->object);
}

static int db_tarantool16_update(struct nb_db *db, struct nb_key *key)
{
	struct db_tarantool16 *t = db->priv;

	tnt_object_reset(t->object);
	tnt_object_add_str(t->object, t->value, t->value_size);

	tnt_update_container_reset(t->update_buf);
	tnt_update_assign(t->update_buf, 2, t->object);
	tnt_update_container_close(t->update_buf);

	tnt_object_reset(t->object);
	tnt_object_add_array(t->object, 1);
	encode_key(t->object, key);
	t->stream->reqid = nb.opts.get_time();

	return tnt_update(t->stream, 512, 0, t->object, t->update_buf);
}

static int db_tarantool16_select(struct nb_db *db, struct nb_key *key)
{
	struct db_tarantool16 *t = db->priv;

	tnt_object_reset(t->object);
	tnt_object_add_array(t->object, 1);
	encode_key(t->object, key);
	t->stream->reqid = nb.opts.get_time();

	return tnt_select(t->stream, 512, 0, 1024, 0, 0, t->object);
}

static int db_tarantool16_msg_len(const char *buf, size_t size)
{
	if (size < 5) {
		return 5;
	}
	if (mp_typeof(*buf) != MP_UINT)
		return -1;
	size_t length = mp_decode_uint(&buf);
	return length + 5;
}

static int db_tarantool16_recv_from_buf(char *buf, size_t size, size_t *off,
					uint64_t *latency)
{
	struct tnt_reply reply;
	int rc = tnt_reply(&reply, buf, size, off);
	if (reply.code != 0) {
		printf("server responded: %d, %-.*s\n", (int)reply.code,
		       (int)(reply.error_end - reply.error), reply.error);
	}
	if (rc)
		return rc;
	if (latency)
		*latency = nb.opts.get_time() - reply.sync;
	return 0;
}

static int db_tarantool16_recv(struct nb_db *db, int count, int *missed,
			       void (*latency_cb)(void *arg, uint64_t lat),
			       void *lat_arg)
{
	(void)missed;
	struct db_tarantool16 *t = db->priv;
	int rc = tnt_flush(t->stream);
	if (rc == -1) {
		printf("sync failed\n");
		return -1;
	}
	struct tnt_iter it; tnt_iter_reply(&it, t->stream);
	while (tnt_next(&it) && count-- > 0) {
		struct tnt_reply *r = TNT_IREPLY_PTR(&it);
		if (r->code != 0) {
			printf("server responded: %d, %-.*s\n", (int)r->code,
			       (int)(r->error_end - r->error), r->error);
		}
		if (latency_cb)
			latency_cb(lat_arg, nb.opts.get_time() - r->sync);
	}
	if (it.status == TNT_ITER_FAIL) {
		if (TNT_SNET_CAST(t->stream)->error) {
			printf("failed to recv answer: %s\n",
			       tnt_strerror(t->object));
		} else {
			printf("failed to parse response\n");
		}
		return -1;
	}

	return 0;
}

static int db_tarantool16_get_fd(struct nb_db *db)
{
	struct db_tarantool16 *t = db->priv;
	struct tnt_stream_net *sn = TNT_SNET_CAST(t->stream);
	return sn->fd;
}

struct nb_db_if nb_db_tarantool16 =
{
	.name     = "tarantool1_6",
	.init     = db_tarantool16_init,
	.free     = db_tarantool16_free,
	.connect  = db_tarantool16_connect,
	.close    = db_tarantool16_close,
	.insert   = db_tarantool16_insert,
	.replace  = db_tarantool16_replace,
	.del      = db_tarantool16_delete,
	.update   = db_tarantool16_update,
	.select   = db_tarantool16_select,
	.recv     = db_tarantool16_recv,
	.get_fd   = db_tarantool16_get_fd,
	.recv_from_buf = db_tarantool16_recv_from_buf,
	.msg_len  = db_tarantool16_msg_len,
	.get_buf  = db_tarantool16_get_buf
};
