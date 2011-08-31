
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

#include <tnt.h>

#include <nosqlb_opt.h>

void
nosqlb_opt_init(struct nosqlb_opt *opt)
{
	opt->proto = TNT_PROTO_RW;
	opt->host = "localhost";
	opt->port = 33013;
	opt->threads = 1;
	opt->count = 1000;
	opt->rep = 1;
	opt->full = 0;
	opt->per = opt->count / opt->threads;
	opt->rbuf = 16384;
	opt->sbuf = 16384;
	opt->plot = 0;
	opt->plot_dir = "benchmark";
	opt->std = 0;
	opt->std_memcache = 0;
	STAILQ_INIT(&opt->tests);
	opt->tests_count = 0;
	STAILQ_INIT(&opt->bufs);
	opt->bufs_count = 0;
}

void
nosqlb_opt_free(struct nosqlb_opt *opt)
{
	struct nosqlb_opt_arg *a, *anext;
	STAILQ_FOREACH_SAFE(a, &opt->tests, next, anext) {
		free(a->arg);
		free(a);
	}
	STAILQ_FOREACH_SAFE(a, &opt->bufs, next, anext) {
		free(a->arg);
		free(a);
	}
}
