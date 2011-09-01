#ifndef NOSQLB_ARG_H_INCLUDED
#define NOSQLB_ARG_H_INCLUDED

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
	NOSQLB_ARG_DONE,
	NOSQLB_ARG_ERROR,
	NOSQLB_ARG_UNKNOWN,
	NOSQLB_ARG_SERVER_HOST,
	NOSQLB_ARG_SERVER_PORT,
	NOSQLB_ARG_THREADS,
	NOSQLB_ARG_BUF_RECV,
	NOSQLB_ARG_BUF_SEND,
	NOSQLB_ARG_AUTH_TYPE,
	NOSQLB_ARG_AUTH_ID,
	NOSQLB_ARG_AUTH_KEY,
	NOSQLB_ARG_AUTH_MECH,
	NOSQLB_ARG_TEST_STD,
	NOSQLB_ARG_TEST_STD_MC,
	NOSQLB_ARG_TEST,
	NOSQLB_ARG_TEST_BUF,
	NOSQLB_ARG_TEST_BUF_FILE,
	NOSQLB_ARG_TEST_LIST,
	NOSQLB_ARG_COUNT,
	NOSQLB_ARG_REP,
	NOSQLB_ARG_FULL,
	NOSQLB_ARG_TOW,
	NOSQLB_ARG_PLOT,
	NOSQLB_ARG_PLOT_DIR,
	NOSQLB_ARG_HELP
};

struct nosqlb_arg_cmd {
	char *name;
	int arg;
	int token;
};

struct nosqlb_arg {
	int pos;
	int argc;
	char **argv;
	struct nosqlb_arg_cmd *cmds;
};

void nosqlb_arg_init(struct nosqlb_arg *arg,
		     struct nosqlb_arg_cmd *cmds, int argc, char * argv[]);

int nosqlb_arg(struct nosqlb_arg *arg, char **argp);

#endif /* NOSQLB_ARG_H_INCLUDED */
