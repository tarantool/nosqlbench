
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

#include <tarantool/tnt.h>
#include <tarantool/tnt_net.h>

#include "nb_alloc.h"
#include "nb_key.h"
#include "nb_db.h"
#include "nb_db_tarantool.h"

struct db_tarantool {
	struct tnt_stream s;
	char *value;
	size_t value_size;
};

static int db_tarantool_init(struct nb_db *db, size_t value_size) {
	db->priv = nb_malloc(sizeof(struct db_tarantool));
	memset(db->priv, 0, sizeof(struct db_tarantool));
	struct db_tarantool *t = db->priv;
	t->value_size = value_size;
	t->value = nb_malloc(t->value_size);
	memset(t->value, '#', t->value_size - 1);
	t->value[t->value_size - 1] = 0;
	return 0;
}

static void db_tarantool_free(struct nb_db *db)
{
	struct db_tarantool *t = db->priv;
	tnt_stream_free(&t->s);
	if (t->value)
		free(t->value);
	free(db->priv);
	db->priv = NULL;
}

static int db_tarantool_connect(struct nb_db *db, char *host, int port)
{
	struct db_tarantool *t = db->priv;
	if (tnt_net(&t->s) == NULL) {
		printf("tnt_stream_net() failed\n");
		return -1;
	}
	tnt_set(&t->s, TNT_OPT_HOSTNAME, host);
	tnt_set(&t->s, TNT_OPT_PORT, port);
	tnt_set(&t->s, TNT_OPT_SEND_BUF, 0);
	tnt_set(&t->s, TNT_OPT_RECV_BUF, 0);
	if (tnt_init(&t->s) == -1)
		return -1;
	if (tnt_connect(&t->s) == -1) {
		printf("tnt_connect() failed: %s\n", tnt_strerror(&t->s));
		return -1;
	}
	return 0;
}

static void db_tarantool_close(struct nb_db *db)
{
	struct db_tarantool *t = db->priv;
	tnt_close(&t->s);
}

static int db_tarantool_insert(struct nb_db *db, struct nb_key *key)
{
	struct db_tarantool *t = db->priv;
	struct tnt_tuple tu;
	tnt_tuple_init(&tu);
	tnt_tuple_add(&tu, key->data, key->size);
	tnt_tuple_add(&tu, t->value, t->value_size);
	if (tnt_insert(&t->s, 0, 0, &tu) == -1) {
		tnt_tuple_free(&tu);
		return -1;
	}
	tnt_tuple_free(&tu);
	return 0;
}

static int db_tarantool_replace(struct nb_db *db, struct nb_key *key)
{
	struct db_tarantool *t = db->priv;
	struct tnt_tuple tu;
	tnt_tuple_init(&tu);
	tnt_tuple_add(&tu, key->data, key->size);
	tnt_tuple_add(&tu, t->value, t->value_size);
	if (tnt_insert(&t->s, 0, TNT_FLAG_REPLACE, &tu) == -1) {
		tnt_tuple_free(&tu);
		return -1;
	}
	tnt_tuple_free(&tu);
	return 0;
}

static int db_tarantool_update(struct nb_db *db, struct nb_key *key)
{
	struct db_tarantool *t = db->priv;
	struct tnt_tuple tu;
	tnt_tuple_init(&tu);
	tnt_tuple_add(&tu, key->data, key->size);
	struct tnt_stream u;
	tnt_buf(&u);
	tnt_update_assign(&u, 1, t->value, t->value_size);
	if (tnt_update(&t->s, 0, 0, &tu, &u) == -1) {
		tnt_stream_free(&u);
		tnt_tuple_free(&tu);
		return -1;
	}
	tnt_stream_free(&u);
	tnt_tuple_free(&tu);
	return 0;
}

static int db_tarantool_delete(struct nb_db *db, struct nb_key *key)
{
	struct db_tarantool *t = db->priv;
	struct tnt_tuple tu;
	tnt_tuple_init(&tu);
	tnt_tuple_add(&tu, key->data, key->size);
	if (tnt_delete(&t->s, 0, 0, &tu) == -1) {
		tnt_tuple_free(&tu);
		return -1;
	}
	tnt_tuple_free(&tu);
	return 0;
}

static int db_tarantool_select(struct nb_db *db, struct nb_key *key)
{
	struct db_tarantool *t = db->priv;
	struct tnt_tuple tu;
	tnt_tuple_init(&tu);
	tnt_tuple_add(&tu, key->data, key->size);
	struct tnt_list l;
	memset(&l, 0, sizeof(l));
	tnt_list_at(&l, &tu);
	if (tnt_select(&t->s, 0, 0, 0, 100, &l) == -1) {
		tnt_list_free(&l);
		tnt_tuple_free(&tu);
		return -1;
	}
	tnt_list_free(&l);
	tnt_tuple_free(&tu);
	return 0;
}

static int db_tarantool_recv(struct nb_db *db, int count, int *missed)
{
	struct db_tarantool *t = db->priv;
	struct tnt_iter i;
	tnt_iter_reply(&i, &t->s);
	while (tnt_next(&i) && count-- >= 0) {
		struct tnt_reply *r = TNT_IREPLY_PTR(&i);
		if (tnt_error(&t->s) != TNT_EOK) {
			printf("error, %s\n", tnt_strerror(&t->s));
		} else if (r->code != 0) {
			if (r->code == 12546) /* tuple is missing */
				*missed = *missed + 1;
			else
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
		printf("recv failed: %s\n", tnt_strerror(&t->s));
	tnt_iter_free(&i);
	return 0;
}

struct nb_db_if nb_db_tarantool =
{
	.name    = "tarantool",
	.init    = db_tarantool_init,
	.free    = db_tarantool_free,
	.connect = db_tarantool_connect,
	.close   = db_tarantool_close,
	.insert  = db_tarantool_insert,
	.replace = db_tarantool_replace,
	.del     = db_tarantool_delete,
	.update  = db_tarantool_update,
	.select  = db_tarantool_select,
	.recv    = db_tarantool_recv
};
