
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
#include <pthread.h>

#include <leveldb/c.h>

#include "nb_alloc.h"
#include "nb_key.h"
#include "nb_opt.h"
#include "nb_db.h"
#include "nb_db_leveldb.h"

/*
 * Since nosqlbench is mostly oriented on network databases, we have no
 * good way to represent local (embedded) behaviour of leveldb.
 *
 * According to leveldb manual, a database may only be opened by one process at
 * a time. Within a single process, the same leveldb_db_t object may be safely
 * shared by multiple concurrent threads. I.e., different threads may write into
 * or fetch iterators or call Get on the same database without any external sync
 * (the leveldb implementation will automatically do the required sync).
 *
 * In order to deal with it, we maintain an single shared instance (connection)
 * with reference counter. The connection is automatically initialized when
 * first worker tries to connect and automatically destroyed when last worker
 * calls close. Since there is only one leveldb instance, host and port params
 * are meaningless. However, we make some sanity check for cases, when somebody
 * would try to use this backend with various (host, port) settings. All data is
 * saved to DATA_PATH directitory on local host.
 */

/* A path where leveldb data will be stored. Formatted with host, port */
static const char *DATA_PATH = "/var/tmp/nb-leveldb-%s-%d";

struct leveldb_instance {
	leveldb_t *db;
	leveldb_options_t* options;
	leveldb_readoptions_t *roptions;
	leveldb_writeoptions_t *woptions;
	char *host;
	unsigned short int port;
	size_t ref_count;
};

static struct leveldb_instance instance;
static int instance_initialized = 0;
static pthread_mutex_t instance_lock = PTHREAD_MUTEX_INITIALIZER;

struct db_leveldb {
	struct leveldb_instance *instance;
	char *value;
	size_t value_size;
	int sent; /* emulate nb recv api */
};

static int db_leveldb_instance_init(char *host, int port)
{
	instance.options = leveldb_options_create();
	instance.roptions = leveldb_readoptions_create();
	instance.woptions = leveldb_writeoptions_create();

	leveldb_options_set_compression(instance.options, leveldb_no_compression);
	leveldb_options_set_info_log(instance.options, NULL);
	leveldb_options_set_create_if_missing(instance.options, 1);

	/* do not use cache for our benchmark */
	leveldb_readoptions_set_fill_cache(instance.roptions, 0);

	size_t path_len = strlen(DATA_PATH) + strlen(host) + 5;
	char *path = nb_malloc(path_len);
	snprintf(path, path_len, DATA_PATH, host, port);

	printf("LevelDB init: path=%s\n", path);

	char* err = NULL;
	instance.db = leveldb_open(instance.options, path, &err);
	if (err != NULL) {
		printf("leveldb_open() failed: %s\n", err);
		instance.db = NULL;
		free(path);
		return -1;
	}
	free(path);

	instance.host = strdup(host);
	instance.port = port;
	instance.ref_count = 0;

	return 0;
}

static int db_leveldb_instance_free()
{
	leveldb_close(instance.db);
	leveldb_readoptions_destroy(instance.roptions);
	leveldb_writeoptions_destroy(instance.woptions);
	leveldb_options_destroy(instance.options);

	instance.db = NULL;

	printf("LevelDB free\n");

	free(instance.host);

	return 0;
}

static int db_leveldb_init(struct nb_db *db, size_t value_size) {
	db->priv = nb_malloc(sizeof(struct db_leveldb));
	memset(db->priv, 0, sizeof(struct db_leveldb));
	struct db_leveldb *t = db->priv;
	t->value_size = value_size;
	t->value = nb_malloc(t->value_size);
	memset(t->value, '#', t->value_size - 1);
	t->value[t->value_size - 1] = 0;

	return 0;
}

static void db_leveldb_free(struct nb_db *db)
{
	struct db_leveldb *t = db->priv;

	if (t->value)
		free(t->value);
	free(db->priv);
	db->priv = NULL;
}

static int db_leveldb_connect(struct nb_db *db, struct nb_options *opts)
{
	if (!instance_initialized) {
		pthread_mutex_lock(&instance_lock);
		if (!instance_initialized) {
			db_leveldb_instance_init(opts->host, opts->port);
		}

		instance_initialized = 1;
		pthread_mutex_unlock(&instance_lock);
	}

	if (instance.db == NULL) {
		return -1;
	}

	if (strcmp(instance.host, opts->host) != 0 || instance.port != opts->port) {
		printf("LevelDB backend does not support different (host, port)");
		return -1;
	}

	pthread_mutex_lock(&instance_lock);
	instance.ref_count++;
	pthread_mutex_unlock(&instance_lock);

	struct db_leveldb *t = db->priv;
	t->instance = &instance;

	t->sent = 0;

	return 0;
}

static void db_leveldb_close(struct nb_db *db)
{
	struct db_leveldb *t = db->priv;

	pthread_mutex_lock(&instance_lock);
	instance.ref_count--;
	pthread_mutex_unlock(&instance_lock);

	if (instance.ref_count == 0) {
		pthread_mutex_lock(&instance_lock);
		if (instance.ref_count == 0) {
			db_leveldb_instance_free();
			instance_initialized = 0;
		}
		pthread_mutex_unlock(&instance_lock);
	}

	t->instance = NULL;
}

static int db_leveldb_insert(struct nb_db *db, struct nb_key *key)
{
	struct db_leveldb *t = db->priv;

	char *err = NULL;

	leveldb_put(t->instance->db, t->instance->woptions, key->data, key->size,
		    t->value, t->value_size, &err);
	if (err != NULL) {
		printf("leveldb_put() failed: %s\n", err);
		return -1;
	}

	++t->sent;
	return 0;
}

static int db_leveldb_replace(struct nb_db *db, struct nb_key *key)
{
	struct db_leveldb *t = db->priv;
	char *err = NULL;

	leveldb_delete(t->instance->db, t->instance->woptions,
		       key->data, key->size, &err);
	if (err != NULL) {
		printf("leveldb_delete() failed: %s\n", err);
		return -1;
	}

	leveldb_put(t->instance->db, t->instance->woptions, key->data, key->size,
		    t->value, t->value_size, &err);
	if (err != NULL) {
		printf("leveldb_put() failed: %s\n", err);
		return -1;
	}

	++t->sent;
	return 0;
}

static int db_leveldb_update(struct nb_db *db, struct nb_key *key)
{
	(void) db;
	(void) key;

	/* update is meaningless for leveldb, because it is equivalent to replace */
	printf("leveldb update is not supported\n");
	return -1;
}

static int db_leveldb_delete(struct nb_db *db, struct nb_key *key)
{
	struct db_leveldb *t = db->priv;
	char *err = NULL;

	leveldb_delete(t->instance->db, t->instance->woptions,
		       key->data,
		       key->size, &err);
	if (err != NULL) {
		printf("leveldb_delete() failed: %s\n", err);
		return -1;
	}

	++t->sent;
	return 0;
}

static int db_leveldb_select(struct nb_db *db, struct nb_key *key)
{
	struct db_leveldb *t = db->priv;

	char *err = NULL;

	size_t value_size;
	char* value;
	value = leveldb_get(t->instance->db, t->instance->roptions,
			    key->data, key->size,
			    &value_size, &err);
	if (value) {
		free(value);
	}

	if (err != NULL) {
		printf("leveldb_get() failed: %s\n", err);
		return -1;
	}

	++t->sent;
	return 0;
}

static int db_leveldb_recv(struct nb_db *db, int count, int *missed)
{
	struct db_leveldb *t = db->priv;

	if (t->sent >= count) {
		*missed = 0;
		t->sent -= count;
	} else {
		*missed = count - t->sent;
		t->sent = 0;
	}

	return 0;
}

struct nb_db_if nb_db_leveldb =
{
	.name    = "leveldb",
	.init    = db_leveldb_init,
	.free    = db_leveldb_free,
	.connect = db_leveldb_connect,
	.close   = db_leveldb_close,
	.insert  = db_leveldb_insert,
	.replace = db_leveldb_replace,
	.del     = db_leveldb_delete,
	.update  = db_leveldb_update,
	.select  = db_leveldb_select,
	.recv    = db_leveldb_recv
};
