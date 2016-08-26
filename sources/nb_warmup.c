
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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include "nosqlbench.h"
#include "async_io.h"

extern struct nb nb;
struct nb_db db;

extern volatile sig_atomic_t nb_signaled;

struct io_user_data {
	struct nb_key *key;
	size_t i;
};

static void *io_write(struct async_io_object *io_obj, size_t *size)
{
	struct io_user_data *ud = (struct io_user_data *)async_io_get_user_data(io_obj);
	if ((int)ud->i >= nb.opts.request_count || nb_signaled) {
		async_io_finish(io_obj);
		return NULL;
	}
	nb.key->generate_by_id(ud->key, ud->i);
	nb.db->replace(&db, ud->key);
	ud->i++;
	if (nb.report->progress)
		nb.report->progress(ud->i, nb.opts.request_count);
	return nb.db->get_buf(&db, size);
}

static int io_msg_len(struct async_io_object *io_obj, void *buf, size_t size)
{
	(void)io_obj;
	return nb.db->msg_len(buf, size);
}

static int io_recv_from_buf(struct async_io_object *obj, char *buf, size_t size, size_t *off)
{
	(void)obj;
	return nb.db->recv_from_buf(buf, size, off, NULL);
}

int nb_warmup(void)
{
	int rc = 0;
	db.dif = nb.db;
	db.priv = NULL;

	struct nb_key key;
	nb.key->init(&key, nb.key_dist);

	nb.db->init(&db, nb.opts.value_size);
	if (nb.db->connect(&db, &nb.opts) == -1) {
		rc = 1;
		goto error;
	}
	struct io_user_data userdata = {&key, 0};
	struct async_io_if io_if = {io_msg_len, io_write, io_recv_from_buf};
	struct async_io_object *io_object = async_io_new(nb.db->get_fd(&db), &io_if, &userdata);
	if (io_object == NULL)
		goto error;
	async_io_start(io_object);

	async_io_delete(io_object);
error:
	if (nb.report->progress)
		nb.report->progress(0, 0);

	nb.db->close(&db);
	nb.db->free(&db);
	nb.key->free(&key);
	return rc;
}
