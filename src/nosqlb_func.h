#ifndef NOSQLB_FUNC_H_INCLUDED
#define NOSQLB_FUNC_H_INCLUDED

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

struct nosqlb_func {
	char *name;
	void (*func)(struct tnt *t, int bsize,
		     int count, struct nosqlb_stat *stat);
	STAILQ_ENTRY(nosqlb_func) next;
};

struct nosqlb_funcs {
	int count;
	STAILQ_HEAD(,nosqlb_func) list;
};

void nosqlb_func_init(struct nosqlb_funcs *funcs);
void nosqlb_func_free(struct nosqlb_funcs *funcs);

struct nosqlb_func*
nosqlb_func_add(struct nosqlb_funcs *funcs,
		char *name,
		 void (*func)(struct tnt *t, int bsize,
			      int count, struct nosqlb_stat *stat));

struct nosqlb_func*
nosqlb_func_match(struct nosqlb_funcs *funcs, char *name);

#endif /* NOSQLB_FUNC_H_INCLUDED */
