
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
#include <ctype.h>

#include <sys/types.h>
#include <sys/uio.h>

#include <tarantool/tnt.h>
#include <tarantool/tnt_net.h>
#include <tarantool/tnt_io.h>

#include <nb_queue.h>
#include <nb_stat.h>
#include <nb_func.h>
#include <nb_cb.h>
#include <nb_io.h>
#include <nb_redis.h>

int
nb_mc_set(struct tnt_stream *t, char *key, char *data, int data_size)
{
	char buf[64];
	int len = snprintf(buf, sizeof(buf), "set %s 0 0 %d\r\n", key, data_size);
	struct iovec v[3];
	v[0].iov_base = buf;
	v[0].iov_len  = len;
	v[1].iov_base = data;
	v[1].iov_len  = data_size;
	v[2].iov_base = "\r\n";
	v[2].iov_len  = 2;
	int r = tnt_io_sendv(TNT_SNET_CAST(t), v, 3);
	tnt_flush(t);
	return (r < 0) ? -1 : 0;
}

int
nb_mc_set_recv(struct tnt_stream *t)
{
	return nb_io_expect(t, "STORED\r\n");
}

int
nb_mc_get(struct tnt_stream *t, char *key)
{
	char buf[64];
	int len = snprintf(buf, sizeof(buf), "get %s\r\n", key);
	struct iovec v[1];
	v[0].iov_base = buf;
	v[0].iov_len = len;
	int r = tnt_io_sendv(TNT_SNET_CAST(t), v, 1);
	return (r < 0) ? -1 : 0;
}

int
nb_mc_get_recv(struct tnt_stream *t, char **data, int *data_size)
{
	struct tnt_stream_net *sn = TNT_SNET_CAST(t);
	/* VALUE <key> <flags> <bytes> [<cas unique>]\r\n
	 * <data block>\r\n
	 * ...
	 * END\r\n
	*/
	*data = NULL;
	if (nb_io_expect(t, "VALUE ") == -1)
		return -1;
	/* key */
	int key_len = 0;
	char key[128], ch[1];
	for (;; key_len++) {
		if (key_len > (int)sizeof(key)) {
			sn->error = TNT_EBIG;
			goto error;
		}
		if (nb_io_getc(t, ch) == -1)
			goto error;
		if (ch[0] == ' ') {
			key[key_len] = 0;
			break;
		}
		key[key_len] = ch[0];
	}
	/* flags */
	int flags = 0;
	while (1) {
		if (nb_io_getc(t, ch) == -1)
			goto error;
		if (!isdigit(ch[0])) {
			if (ch[0] == ' ')
				break;
			sn->error = TNT_EFAIL;
			goto error;
		}
		flags *= 10;
		flags += ch[0] - 48;
	}
	/* bytes */
	int value_size = 0;
	while (1) {
		if (nb_io_getc(t, ch) == -1)
			goto error;
		if (!isdigit(ch[0])) {
			if (ch[0] == '\r')
				break;
			sn->error = TNT_EFAIL;
			goto error;
		}
		value_size *= 10;
		value_size += ch[0] - 48;
	}
	/* \n */
	if (nb_io_getc(t, ch) == -1)
		goto error;
	/* data */
	*data_size = value_size;
	*data = tnt_mem_alloc(*data_size);
	if (*data == NULL) {
		sn->error = TNT_EMEMORY;
		goto error;
	}
	if (tnt_io_recv(sn, *data, *data_size) == -1)
		goto error;
	if (nb_io_expect(t, "\r\n") == -1)
		goto error;
	if (nb_io_expect(t, "END\r\n") == -1)
		goto error;
	return 0;
error:
	if (*data)
		tnt_mem_free(*data);
	return -1;
}
