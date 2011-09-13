#ifndef NB_ARG_H_INCLUDED
#define NB_ARG_H_INCLUDED

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

enum {
	NB_ARG_DONE,
	NB_ARG_ERROR,
	NB_ARG_UNKNOWN,
	NB_ARG_SERVER_HOST,
	NB_ARG_SERVER_PORT,
	NB_ARG_THREADS,
	NB_ARG_BUF_RECV,
	NB_ARG_BUF_SEND,
	NB_ARG_AUTH_TYPE,
	NB_ARG_AUTH_ID,
	NB_ARG_AUTH_KEY,
	NB_ARG_AUTH_MECH,
	NB_ARG_TEST_STD,
	NB_ARG_TEST_STD_MC,
	NB_ARG_TEST,
	NB_ARG_TEST_BUF,
	NB_ARG_TEST_BUF_FILE,
	NB_ARG_TEST_BUF_ZONE,
	NB_ARG_TEST_LIST,
	NB_ARG_COUNT,
	NB_ARG_REP,
	NB_ARG_FULL,
	NB_ARG_ESYNC,
	NB_ARG_PLOT,
	NB_ARG_PLOT_LOAD,
	NB_ARG_SAVE,
	NB_ARG_INFO,
	NB_ARG_INFO_INT,
	NB_ARG_HELP
};

struct nb_arg_cmd {
	char *name;
	int argc;
	int token;
};

struct nb_arg {
	int pos;
	int argc;
	char **argv;
	struct nb_arg_cmd *cmds;
};

void nb_arg_init(struct nb_arg *arg,
		 struct nb_arg_cmd *cmds, int argc, char * argv[]);

int nb_arg(struct nb_arg *arg, int *argc, char ***argv);

#endif /* NB_ARG_H_INCLUDED */
