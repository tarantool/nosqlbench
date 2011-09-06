#ifndef NOSQLB_OPT_H_INCLUDED
#define NOSQLB_OPT_H_INCLUDED

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

struct nosqlb_opt_test {
	char *test;
	STAILQ_ENTRY(nosqlb_opt_test) next;
};

struct nosqlb_opt_buf {
	int buf;
	STAILQ_ENTRY(nosqlb_opt_buf) next;
};

struct nosqlb_opt {
	char *host;
	int port;
	int threads;
	enum tnt_proto proto;
	int rbuf;
	int sbuf;
	int count;
	int rep;
	int full;
	int tow;
	int per;
	int plot;
	char *plot_dir;
	int std;
	int std_memcache;
	STAILQ_HEAD(,nosqlb_opt_test) tests;
	int tests_count;
	STAILQ_HEAD(,nosqlb_opt_buf) bufs;
	int bufs_count;
	char *buf_file;
};

void nosqlb_opt_init(struct nosqlb_opt *opt);
void nosqlb_opt_free(struct nosqlb_opt *opt);

#endif /* NOSQLB_OPT_H_INCLUDED */
