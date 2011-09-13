
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

#include <nb_arg.h>

void
nb_arg_init(struct nb_arg *arg,
	    struct nb_arg_cmd *cmds, int argc, char * argv[])
{
	arg->argc = argc;
	arg->argv = argv;
	arg->cmds = cmds;
	arg->pos  = 0;
}

static struct nb_arg_cmd*
nb_arg_cmp(struct nb_arg *arg, char *argument)
{
	int iter = 0;
	for (;arg->cmds[iter].name ; iter++) {
		if (strcmp(arg->cmds[iter].name, argument) == 0)
			return (&arg->cmds[iter]);
	}
	return NULL;
}

int
nb_arg(struct nb_arg *arg, int *argc, char ***argv)
{
	struct nb_arg_cmd *cmd;
	for (arg->pos++ ; arg->pos < arg->argc ; arg->pos++) {
		cmd = nb_arg_cmp(arg, arg->argv[arg->pos]);
		if (cmd == NULL) {
			arg->argc = (arg->argc - arg->pos);
			arg->argv = (arg->argv + arg->pos);
			return NB_ARG_UNKNOWN;
		}
		*argc = 0;
		*argv = NULL;
		if (cmd->argc == 0)
			return cmd->token;
		if ((arg->pos + 1) < arg->argc) {
			*argc = arg->argc;
			*argv = arg->argv + arg->pos + 1;
			arg->pos += cmd->argc;
			return cmd->token;
		} else
			return NB_ARG_ERROR;
	}
	return NB_ARG_DONE;
}
