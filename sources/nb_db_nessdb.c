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
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>

#include <engine/db.h>

#include "config.h"
#include "nb_alloc.h"
#include "nb_key.h"
#include "nb_opt.h"
#include "nb_db.h"
#include "nb_db_nessdb.h"

#if !defined(HAVE_NESSDB_V1) && !defined(HAVE_NESSDB_V2)
#error Unsupported NESSDB version
#endif

/* See comment about local databases in nb_db_leveldb.c */

/* A path where leveldb data will be stored. Formatted with host, port */
static const char *DATA_PATH = "/var/tmp/nb-nessdb-%s-%d";

struct nessdb_instance {
	struct nessdb *db;
};

static struct nessdb_instance instance;
static int instance_initialized = 0;
static pthread_mutex_t instance_lock = PTHREAD_MUTEX_INITIALIZER;

struct db_nessdb {
	struct nessdb_instance *instance;
	char *value;
	size_t value_size;
	int sent; /* emulate nb recv api */
};

static int db_nessdb_instance_init(char *host, int port)
{
	size_t path_len = strlen(DATA_PATH) + strlen(host) + 5;
	char *path = nb_malloc(path_len);
	snprintf(path, path_len, DATA_PATH, host, port);

	printf("nessDB init: path=%s\n", path);

#if defined(HAVE_NESSDB_V2)
	instance.db = db_open(path);
#elif defined(HAVE_NESSDB_V1)
	instance.db = db_open(path, 0UL, 0);
#endif
	if (instance.db == NULL) {
		printf("db_open() failed\n");
		free(path);
		return -1;
	}
	free(path);

	return 0;
}

static int db_nessdb_instance_free()
{
	db_close(instance.db);
	instance.db = NULL;

	printf("nessDB free\n");

	return 0;
}

static int db_nessdb_init(struct nb_db *db, size_t value_size) {
	db->priv = nb_malloc(sizeof(struct db_nessdb));
	memset(db->priv, 0, sizeof(struct db_nessdb));
	struct db_nessdb *t = db->priv;
	t->value_size = value_size;
	t->value = nb_malloc(t->value_size);
	memset(t->value, '#', t->value_size - 1);
	t->value[t->value_size - 1] = 0;

	return 0;
}

static void db_nessdb_free(struct nb_db *db)
{
	struct db_nessdb *t = db->priv;

	if (t->value)
		free(t->value);
	free(db->priv);
	db->priv = NULL;
}

static int db_nessdb_connect(struct nb_db *db, struct nb_options *opts)
{
	pthread_mutex_lock(&instance_lock);
	if (instance_initialized) {
		printf("nessDB is single-threaded!\n");
		goto error;
	}

	if (db_nessdb_instance_init(opts->host, opts->port) != 0)
		goto error;

	instance_initialized = 1;

	struct db_nessdb *t = db->priv;
	t->instance = &instance;
	t->sent = 0;

	pthread_mutex_unlock(&instance_lock);

	return 0;
error:
	pthread_mutex_unlock(&instance_lock);
	return -1;
}

static void db_nessdb_close(struct nb_db *db)
{
	struct db_nessdb *t = db->priv;

	assert (instance_initialized);

	pthread_mutex_lock(&instance_lock);
	db_nessdb_instance_free();
	instance_initialized = 0;
	t->instance = NULL;
	pthread_mutex_unlock(&instance_lock);
}

static int db_nessdb_insert(struct nb_db *db, struct nb_key *key)
{
	struct db_nessdb *t = db->priv;

	assert (key->size < UINT_MAX);
	assert (t->value_size < UINT_MAX);

	struct slice nkey, nval;
	nkey.len = key->size;
	nkey.data = key->data;
	nval.len = t->value_size;
	nval.data = t->value;

	int count = db_add(t->instance->db, &nkey, &nval);
	if (count != 1) {
		printf("db_add() failed: %d\n", count);
		return -1;
	}

	++t->sent;
	return 0;
}

static int db_nessdb_replace(struct nb_db *db, struct nb_key *key)
{
	struct db_nessdb *t = db->priv;

	assert (key->size < UINT_MAX);
	assert (t->value_size < UINT_MAX);

	struct slice nkey, nval;
	nkey.len = key->size;
	nkey.data = key->data;
	nval.len = t->value_size;
	nval.data = t->value;

	/* db_remove(t->instance->db, &nkey); */
	int count = db_add(t->instance->db, &nkey, &nval);
	if (count != 1) {
		printf("db_add() failed: %d\n", count);
		return -1;
	}

	++t->sent;
	return 0;
}

static int db_nessdb_update(struct nb_db *db, struct nb_key *key)
{
	(void) db;
	(void) key;

	/* update is meaningless for nessdb, because it is equivalent to replace */
	printf("nessdb update is not supported\n");
	return -1;
}

static int db_nessdb_delete(struct nb_db *db, struct nb_key *key)
{
	struct db_nessdb *t = db->priv;

	assert (key->size < UINT_MAX);

	struct slice nkey;
	nkey.len = key->size;
	nkey.data = key->data;

	db_remove(t->instance->db, &nkey);

	++t->sent;
	return 0;
}

static int db_nessdb_select(struct nb_db *db, struct nb_key *key)
{
	struct db_nessdb *t = db->priv;

	assert (key->size < UINT_MAX);

	struct slice nkey, nval;
	nkey.len = key->size;
	nkey.data = key->data;

	int count = db_get(t->instance->db, &nkey, &nval);
	if (count != 1) {
		printf("db_get() failed: %d\n", count);
		return -1;
	}

#if defined(HAVE_NESSDB_SST)
	db_free_data(nval.data);
#endif

	++t->sent;
	return 0;
}

static int db_nessdb_recv(struct nb_db *db, int count, int *missed)
{
	struct db_nessdb *t = db->priv;

	assert (t->sent >= count);
	*missed = 0;
	t->sent -= count;

	return 0;
}

struct nb_db_if nb_db_nessdb =
{
	.name    = "nessdb",
	.init    = db_nessdb_init,
	.free    = db_nessdb_free,
	.connect = db_nessdb_connect,
	.close   = db_nessdb_close,
	.insert  = db_nessdb_insert,
	.replace = db_nessdb_replace,
	.del     = db_nessdb_delete,
	.update  = db_nessdb_update,
	.select  = db_nessdb_select,
	.recv    = db_nessdb_recv
};
