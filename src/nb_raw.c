
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

#include <tnt.h>

#include <nb_stat.h>
#include <nb_func.h>
#include <nb_cb.h>
#include <nb_raw.h>

int tnt_leb128_read(char *buf, int size, uint32_t *value);
void tnt_leb128_write(char *buf, uint32_t value);
int tnt_leb128_size(uint32_t value);

int
nb_raw_insert(struct tnt *t, char *key, char *data, int data_size)
{
	int key_size = strlen(key) + 1;
	uint32_t cardinality = 2;

	char key_leb[6];
	int key_leb_size = tnt_leb128_size(key_size);
	tnt_leb128_write(key_leb, key_size);

	char data_leb[6];
	int data_leb_size = tnt_leb128_size(data_size);
	tnt_leb128_write(data_leb, data_size);

	struct tnt_proto_header hdr;
	hdr.type  = TNT_PROTO_TYPE_INSERT;
	hdr.reqid = 0;
	hdr.len   = sizeof(struct tnt_proto_insert) +
			4 + 
			key_leb_size +
			key_size +
			data_leb_size +
			data_size;

	struct tnt_proto_insert hdr_insert;
	hdr_insert.ns = 0;
	hdr_insert.flags = 0;

	struct iovec v[7];
	v[0].iov_base = &hdr;
	v[0].iov_len  = sizeof(struct tnt_proto_header);
	v[1].iov_base = &hdr_insert;
	v[1].iov_len  = sizeof(struct tnt_proto_insert);
	v[2].iov_base = &cardinality;
	v[2].iov_len  = sizeof(cardinality);
	v[3].iov_base = key_leb;
	v[3].iov_len  = key_leb_size;
	v[4].iov_base = key;
	v[4].iov_len  = key_size;
	v[5].iov_base = data_leb;
	v[5].iov_len  = data_leb_size;
	v[6].iov_base = data;
	v[6].iov_len  = data_size;
	int r = tnt_io_sendv(t, v, 7);
	if (r < 0) {
		t->error = TNT_ESYSTEM;
		return -1;
	}

	return 0;
}

int
nb_raw_insert_recv(struct tnt *t)
{
	char buffer[sizeof(struct tnt_proto_header_resp) + 4];
	struct tnt_proto_header_resp *hdr =
		(struct tnt_proto_header_resp*)buffer;
	t->error = tnt_io_recv(t, buffer, sizeof(struct tnt_proto_header_resp));
	if (t->error != TNT_EOK)
		return -1;
	if (!TNT_PROTO_IS_OK(hdr->code))
		return -1;
	t->error = tnt_io_recv(t, buffer +
		sizeof(struct tnt_proto_header_resp), 4);
	if (t->error != TNT_EOK)
		return -1;
	int count =  *(uint32_t*)(buffer +
		sizeof(struct tnt_proto_header_resp));
	return (count == 1) ? 0 : -1;
}
