
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

#include "nb_alloc.h"
#include "nb_opt.h"
#include "nb_key.h"
#include "nb_db.h"
#include "nb_db_memcached_bin.h"

#include "memcached/mc.h"
#include "memcached/session.h"

struct db_memcached_bin {
	struct tbses s;
	char *value;
	size_t value_size;
	char *buf;
	size_t buf_size;
};

static int db_memcached_bin_init(struct nb_db *db, size_t value_size)
{
	db->priv = nb_malloc(sizeof(struct db_memcached_bin));
	memset(db->priv, 0, sizeof(struct db_memcached_bin));
	struct db_memcached_bin *t = db->priv;
	t->value_size = value_size;
	t->value = nb_malloc(t->value_size);
	memset(t->value, '#', t->value_size - 1);
	t->value[t->value_size - 1] = 0;
	t->buf_size = 1024 + value_size;
	t->buf = nb_malloc(t->buf_size);
	return 0;
}

static void db_memcached_bin_free(struct nb_db *db)
{
	struct db_memcached_bin *t = db->priv;
	tb_sesclose(&t->s);
	tb_sesfree(&t->s);
	if (t->value) free(t->value);
	if (t->buf) free(t->buf);
	free(db->priv);
	db->priv = NULL;
}
static int db_memcached_bin_connect(struct nb_db *db, struct nb_options *opts)
{
	struct db_memcached_bin *t = db->priv;
	tb_sesset(&t->s, TB_HOST, opts->host);
	tb_sesset(&t->s, TB_PORT, opts->port);
	tb_sesset(&t->s, TB_SENDBUF, opts->buf_send);
	tb_sesset(&t->s, TB_READBUF, opts->buf_recv);
	tb_sesset(&t->s, TB_CONNECTTM, (int )60);
	int rc = tb_sesconnect(&t->s);
	if (rc == -1) {
		printf("memcached_connect() failed: %s\n", strerror(t->s.errno_));
		return -1;
	}
	return 0;
}

static void db_memcached_bin_close(struct nb_db *db)
{
	struct db_memcached_bin *t = db->priv;
	tb_sesclose(&t->s);
}

static int db_memcached_bin_insert(struct nb_db *db, struct nb_key *key)
{
	struct db_memcached_bin *t = db->priv;
	struct mc req;
	mc_init   (&req, t->buf, t->buf_size, NULL, NULL);
	mc_op_add (&req, key->data, key->size, t->value, t->value_size, 0, 0);
	uint32_t size = mc_used(&req);
	assert(size <= t->buf_size);
	int rc = tb_sessend(&t->s, t->buf, size);
	if (rc == -1)
		return -1;
	return 0;
}

static int db_memcached_bin_replace(struct nb_db *db, struct nb_key *key)
{
	struct db_memcached_bin *t = db->priv;
	struct mc req;
	mc_init   (&req, t->buf, t->buf_size, NULL, NULL);
	mc_op_set (&req, key->data, key->size, t->value, t->value_size, 0, 0);
	uint32_t size = mc_used(&req);
	assert(size <= t->buf_size);
	int rc = tb_sessend(&t->s, t->buf, size);
	if (rc == -1)
		return -1;
	return 0;
}

static int db_memcached_bin_delete(struct nb_db *db, struct nb_key *key)
{
	struct db_memcached_bin *t = db->priv;
	struct mc req;
	mc_init      (&req, t->buf, t->buf_size, NULL, NULL);
	mc_op_delete (&req, key->data, key->size);
	uint32_t size = mc_used(&req);
	assert(size <= t->buf_size);
	int rc = tb_sessend(&t->s, t->buf, size);
	if (rc == -1)
		return -1;
	return 0;
}

static int db_memcached_bin_update(struct nb_db *db, struct nb_key *key)
{
	(void) db;
	(void) key;

	/* update is meaningless for memcached, because it is
	 * equivalent to replace */
	printf("memcached update is not supported\n");
	return -1;
}

static int db_memcached_bin_select(struct nb_db *db, struct nb_key *key)
{
	struct db_memcached_bin *t = db->priv;
	struct mc req;
	mc_init   (&req, t->buf, t->buf_size, NULL, NULL);
	mc_op_get (&req, key->data, key->size);
	uint32_t size = mc_used(&req);
	assert(size <= t->buf_size);
	int rc = tb_sessend(&t->s, t->buf, size);
	if (rc == -1)
		return -1;
	return 0;
}

static int db_memcached_bin_recv(struct nb_db *db, int count, int *missed)
{
	struct db_memcached_bin *t = db->priv;
	int rc = tb_sessync(&t->s);
	if (rc == -1) {
		printf("sync failed\n");
		return -1;
	}
	while (count > 0)
	{
		struct mc_response resp;
		ssize_t len = tb_sesrecv(&t->s, t->buf,
					 sizeof(struct mc_hdr), 1);
		if (len < (ssize_t)sizeof(struct mc_hdr)) {
			printf("recv failed\n");
			return 0;
		}
		const char *p = t->buf;
		ssize_t rv = mc_reply(&resp, &p, p + sizeof(struct mc_hdr));
		if (rv == -1) {
			printf("failed to parse response\n");
			return -1;
		} if (rv == 0) {
			goto get;
		}
		len = tb_sesrecv(&t->s, t->buf + sizeof(struct mc_hdr),
				 rv, 1);
		if (len < rv) {
			printf("recv failed\n");
			return -1;
		}
		const char *end = p + sizeof(struct mc_hdr) + rv;
		int64_t r = mc_reply(&resp, &p, end);
		if (r == -1) {
			printf("failed to parse response\n");
			return -1;
		}
get:
		assert(p == end);
		if (resp.hdr.cmd == MC_BIN_CMD_GET && resp.hdr.status)
			*missed = *missed + 1;
		else if (resp.hdr.cmd != MC_BIN_CMD_GET && resp.hdr.status)
			printf("server respond: %d\n", resp.hdr.status);
		count--;
	}
	return 0;
}

struct nb_db_if nb_db_memcached_bin =
{
	.name    = "memcached_bin",
	.init    = db_memcached_bin_init,
	.free    = db_memcached_bin_free,
	.connect = db_memcached_bin_connect,
	.close   = db_memcached_bin_close,
	.insert  = db_memcached_bin_insert,
	.replace = db_memcached_bin_replace,
	.del     = db_memcached_bin_delete,
	.update  = db_memcached_bin_update,
	.select  = db_memcached_bin_select,
	.recv    = db_memcached_bin_recv
};
