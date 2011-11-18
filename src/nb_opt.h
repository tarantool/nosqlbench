#ifndef NB_OPT_H_INCLUDED
#define NB_OPT_H_INCLUDED

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

struct nb_opt_test {
	char *test;
	STAILQ_ENTRY(nb_opt_test) next;
};

struct nb_opt_buf {
	int buf;
	STAILQ_ENTRY(nb_opt_buf) next;
};

enum {
	NB_OPT_DATA_NONE,
	NB_OPT_DATA_SAVE,
	NB_OPT_DATA_PLOT,
	NB_OPT_DATA_INFO,
	NB_OPT_DATA_INFO_INT
};

struct nb_opt {
	char *host;
	uint32_t port;
	uint32_t threads;
	uint32_t rbuf;
	uint32_t sbuf;
	uint32_t count;
	uint32_t rep;
	uint32_t full;
	uint32_t esync;
	uint32_t per;
	char *plot;
	int data;
	char *datap;
	uint32_t std;
	uint32_t std_memcache;
	STAILQ_HEAD(,nb_opt_test) tests;
	uint32_t tests_count;
	STAILQ_HEAD(,nb_opt_buf) bufs;
	uint32_t bufs_count;
	char *buf_file;
};

void nb_opt_init(struct nb_opt *opt);
void nb_opt_free(struct nb_opt *opt);

#endif /* NB_OPT_H_INCLUDED */
