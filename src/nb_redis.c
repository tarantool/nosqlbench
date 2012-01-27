
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

#include <libtnt/tnt.h>
#include <libtnt/tnt_net.h>
#include <libtnt/tnt_io.h>

#include <nb_queue.h>
#include <nb_stat.h>
#include <nb_func.h>
#include <nb_cb.h>
#include <nb_io.h>
#include <nb_redis.h>

int
nb_redis_set(struct tnt_stream *t, char *key, char *data, int data_size)
{
	char buf[64];
	int len = snprintf(buf, sizeof(buf), "SET %s \"", key);
	struct iovec v[3];
	v[0].iov_base = buf;
	v[0].iov_len  = len;
	v[1].iov_base = data;
	v[1].iov_len  = data_size;
	v[2].iov_base = "\"\r\n";
	v[2].iov_len  = 3;
	int r = tnt_io_sendv(TNT_SNET_CAST(t), v, 3);
	return (r < 0) ? -1 : 0;
}

int
nb_redis_set_recv(struct tnt_stream *t)
{
	return nb_io_expect(t, "+OK\r\n");
}

int
nb_redis_get(struct tnt_stream *t, char *key)
{
	char buf[64];
	int len = snprintf(buf, sizeof(buf), "GET %s\r\n", key);
	struct iovec v[1];
	v[0].iov_base = buf;
	v[0].iov_len = len;
	int r = tnt_io_sendv(TNT_SNET_CAST(t), v, 1);
	return (r < 0) ? -1 : 0;
}

int
nb_redis_get_recv(struct tnt_stream *t, char **data, int *data_size)
{
	struct tnt_stream_net *sn = TNT_SNET_CAST(t);
	/*
		GET mykey
		$6\r\nfoobar\r\n
	*/
	if (nb_io_expect(t, "$") == -1)
		return -1;
	*data_size = 0;
	char ch[1];
	while (1) {
		if (nb_io_getc(t, ch) == -1)
			return -1;
		if (!isdigit(ch[0])) {
			if (ch[0] == '\r')
				break;
			sn->error = TNT_EFAIL;
			return -1;
		}
		*data_size *= 10;
		*data_size += ch[0] - 48;
	}

	if (nb_io_getc(t, ch) == -1)
		return -1;
	if (ch[0] != '\n') {
		sn->error = TNT_EFAIL;
		return -1;
	}
	*data = tnt_mem_alloc(*data_size);
	if (*data == NULL) {
		sn->error = TNT_EFAIL;
		return -1;
	}
	if (tnt_io_recv(sn, *data, *data_size) == -1) {
		tnt_mem_free(*data);
		return -1;
	}
	if (nb_io_expect(t, "\r\n") == -1) {
		tnt_mem_free(*data);
		return -1;
	}
	return 0;
}
