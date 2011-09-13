#ifndef NB_FUNC_H_INCLUDED
#define NB_FUNC_H_INCLUDED

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

struct nb_stat;

struct nb_func {
	char *name;
	void (*func)(struct tnt *t, int tid, int bsize, int count,
	             struct nb_stat *stat);
	STAILQ_ENTRY(nb_func) next;
};

struct nb_funcs {
	int count;
	STAILQ_HEAD(,nb_func) list;
};

void nb_func_init(struct nb_funcs *funcs);
void nb_func_free(struct nb_funcs *funcs);

struct nb_func*
nb_func_add(struct nb_funcs *funcs,
	    char *name,
	    void (*func)(struct tnt *t, int tid, int bsize, int count,
			 struct nb_stat *stat));

struct nb_func*
nb_func_match(struct nb_funcs *funcs, char *name);

#endif /* NB_FUNC_H_INCLUDED */
