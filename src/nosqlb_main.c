
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

#include <nosqlb_arg.h>
#include <nosqlb_stat.h>
#include <nosqlb_func.h>
#include <nosqlb_test.h>
#include <nosqlb_opt.h>
#include <nosqlb_cb.h>
#include <nosqlb.h>

static void
nosqlb_usage(struct nosqlb_opt *opts, char *name)
{
	printf("%s [options]\n\n", name);

	printf("NoSQL benchmarking.\n");
	printf("connection:\n");
	printf("  -a, --server-host [host]      server address (%s)\n", opts->host);
	printf("  -p, --server-port [port]      server port (%d)\n", opts->port);
	printf("  -r, --buf-recv [rbuf]         receive buffer size (%d)\n", opts->rbuf);
	printf("  -s, --buf-send [sbuf]         send buffer size (%d)\n", opts->sbuf);
	printf("  -t, --threads [count]         connection threads (%d)\n\n", opts->threads);

	printf("benchmark:\n");
	printf("  -M, --test-std-mc             standart memcache testing set (%d)\n", opts->std_memcache);
	printf("  -A, --test-std-tnt            standart tarantool testing set (%d)\n", opts->std);
	printf("  -T, --test [name,...]         list of tests\n");
	printf("  -B, --test-buf [buf,...]      test buffer sizes\n");
	printf("  -F, --test-buf-file [path]    read tests buffer sizes from file\n");
	printf("  -L, --test-list               list available tests\n");
	printf("  -C, --count [count]           request count (%d)\n", opts->count);
	printf("  -R, --repeat [count]          request repeat (%d)\n", opts->rep);
	printf("  -U, --full                    do full request range per thread (%d)\n", opts->full);
	printf("  -W, --tow                     display total time for each test (%d)\n", opts->tow);
	printf("  -P, --plot                    generate gnuplot files (%d)\n", opts->plot);
	printf("  -D, --plot-dir [path]         plot output directory (%s)\n\n", opts->plot_dir);

	printf("other:\n");
	printf("  -h, --help                    show usage\n\n");

	printf("examples:\n");
	printf("  # standart iproto benchmark\n");
	printf("  nosqlb --test-std-tnt\n\n");

	printf("  # benchmark insert, select for 48, 96, 102 buffers\n");
	printf("  # for 10000 counts\n");
	printf("  nosqlb --test tnt-insert,tnt-select -B 48,96,102 -C 10000\n\n");

	printf("  # benchmark async and sync insert tests\n");
	printf("  nosqlb -T tnt-insert -T tnt-insert-sync -B 32,64,128 -C 100000 -P\n\n");

	printf("  # benchmark memcache protocol for 32, 64, 128 bytes payload\n");
	printf("  # with plot generation\n");
	printf("  nosqlb -T memcache-set -B 32,64,128 -C 10000 -P\n");

	exit(1);
}

static
struct nosqlb_arg_cmd cmds[] =
{
	{ "-h",              0, NOSQLB_ARG_HELP          },
	{ "--help",          0, NOSQLB_ARG_HELP          },
	{ "-a",              1, NOSQLB_ARG_SERVER_HOST   },
	{ "--server-host",   1, NOSQLB_ARG_SERVER_HOST   },
	{ "-p",              1, NOSQLB_ARG_SERVER_PORT   },
	{ "--server-port",   1, NOSQLB_ARG_SERVER_PORT   },
	{ "-t",              1, NOSQLB_ARG_THREADS       },
	{ "--threads",       1, NOSQLB_ARG_THREADS       },
	{ "-r",              1, NOSQLB_ARG_BUF_RECV      },
	{ "--buf-recv",      1, NOSQLB_ARG_BUF_RECV      },
	{ "-s",              1, NOSQLB_ARG_BUF_SEND      },
	{ "--buf-send",      1, NOSQLB_ARG_BUF_SEND      },
	{ "-M",              0, NOSQLB_ARG_TEST_STD_MC   },
	{ "--test-std-mc",   0, NOSQLB_ARG_TEST_STD_MC   },
	{ "-A",              0, NOSQLB_ARG_TEST_STD      },
	{ "--test-std-tnt",  0, NOSQLB_ARG_TEST_STD      },
	{ "-T",              1, NOSQLB_ARG_TEST          },
	{ "--test",          1, NOSQLB_ARG_TEST          },
	{ "-B",              1, NOSQLB_ARG_TEST_BUF      },
	{ "--test-buf",      1, NOSQLB_ARG_TEST_BUF      },
	{ "-F",              1, NOSQLB_ARG_TEST_BUF_FILE },
	{ "--test-buf-file", 1, NOSQLB_ARG_TEST_BUF_FILE },
	{ "-L",              0, NOSQLB_ARG_TEST_LIST     },
	{ "--test-list",     0, NOSQLB_ARG_TEST_LIST     },
	{ "-C",              1, NOSQLB_ARG_COUNT         },
	{ "--count",         1, NOSQLB_ARG_COUNT         },
	{ "-R",              1, NOSQLB_ARG_REP           },
	{ "--repeat",        1, NOSQLB_ARG_REP           },
	{ "-U",              0, NOSQLB_ARG_FULL          },
	{ "--full",          0, NOSQLB_ARG_FULL          },
	{ "-W",              0, NOSQLB_ARG_TOW           },
	{ "--tow",           0, NOSQLB_ARG_TOW           },
	{ "-P",              0, NOSQLB_ARG_PLOT          },
	{ "--plot",          0, NOSQLB_ARG_PLOT          },
	{ "-D",              1, NOSQLB_ARG_PLOT_DIR      },
	{ "--plot-dir",      1, NOSQLB_ARG_PLOT_DIR      },
	{ NULL,              0, 0                        }
};

static void
nosqlb_add_args(struct nosqlb_opt *opts, char *argp, int test)
{
	char buflist[1024];
	strncpy(buflist, argp, sizeof(buflist));
	char *p;
	for (p = strtok(buflist, ",") ; p ; p = strtok(NULL, ",")) {
		struct nosqlb_opt_arg *arg =
			malloc(sizeof(struct nosqlb_opt_arg));
		if (arg == NULL)
			return;
		arg->arg = strdup(p);
		if (test) {
			STAILQ_INSERT_TAIL(&opts->tests, arg, next);
			opts->tests_count++;
		} else {
			STAILQ_INSERT_TAIL(&opts->bufs, arg, next);
			opts->bufs_count++;
		}
	}
}

static void
nosqlb_add_bufs_chop(char *buf, int size)
{
	int i;
	for (i = 0 ; i < size ; i++) {
		if (buf[i] == '\n') {
			buf[i] = 0;
			return;
		}
	}
}

static void
nosqlb_add_bufs(struct nosqlb_opt *opts, char *argp)
{
	FILE *f = fopen(argp, "r");
	if (f == NULL) {
		printf("failed to open file %s\n", argp);
		return;
	}
	char buf[64];
	while (fgets(buf, sizeof(buf), f)) {
		struct nosqlb_opt_arg *arg =
			malloc(sizeof(struct nosqlb_opt_arg));
		if (arg == NULL)
			return;
		nosqlb_add_bufs_chop(buf, sizeof(buf));
		arg->arg = strdup(buf);
		STAILQ_INSERT_TAIL(&opts->bufs, arg, next);
		opts->bufs_count++;
	}
	fclose(f);
}

static void
nosqlb_args(struct nosqlb_funcs * funcs,
	struct nosqlb_opt *opts, int argc, char * argv[])
{
	struct nosqlb_arg args;
	nosqlb_arg_init(&args, cmds, argc, argv);

	while (1) {
		char *argp;
		switch (nosqlb_arg(&args, &argp)) {
		case NOSQLB_ARG_DONE:
			return;
		case NOSQLB_ARG_UNKNOWN:
		case NOSQLB_ARG_ERROR:
		case NOSQLB_ARG_HELP:
			nosqlb_usage(opts, argv[0]);
			break;
		case NOSQLB_ARG_SERVER_HOST:
			opts->host = argp;
			break;
		case NOSQLB_ARG_SERVER_PORT:
			opts->port = atoi(argp);
			break;
		case NOSQLB_ARG_THREADS:
			opts->threads = atoi(argp);
			break;
		case NOSQLB_ARG_BUF_RECV:
			opts->rbuf = atoi(argp);
			break;
		case NOSQLB_ARG_BUF_SEND:
			opts->sbuf = atoi(argp);
			break;
		case NOSQLB_ARG_TEST_STD_MC:
			opts->std_memcache = 1;
			break;
		case NOSQLB_ARG_TEST_STD:
			opts->std = 1;
			break;
		case NOSQLB_ARG_TEST:
			nosqlb_add_args(opts, argp, 1);
			break;
		case NOSQLB_ARG_TEST_BUF:
			nosqlb_add_args(opts, argp, 0);
			break;
		case NOSQLB_ARG_TEST_BUF_FILE:
			nosqlb_add_bufs(opts, argp);
			break;
		case NOSQLB_ARG_TEST_LIST:
			printf("available tests:\n");
			struct nosqlb_func *func;
			STAILQ_FOREACH(func, &funcs->list, next)
				printf("  %s\n", func->name);
			exit(0);
			break;
		case NOSQLB_ARG_COUNT:
			opts->count = atoi(argp);
			break;
		case NOSQLB_ARG_REP:
			opts->rep = atoi(argp);
			break;
		case NOSQLB_ARG_FULL:
			opts->full = 1;
			break;
		case NOSQLB_ARG_TOW:
			opts->tow = 1;
			break;
		case NOSQLB_ARG_PLOT:
			opts->plot = 1;
			break;
		case NOSQLB_ARG_PLOT_DIR:
			opts->plot_dir = argp;
			break;
		}
	}
}

int
main(int argc, char * argv[])
{
	struct nosqlb_funcs funcs;
	nosqlb_func_init(&funcs);
	nosqlb_cb_init(&funcs);

	struct nosqlb_opt opts;
	nosqlb_opt_init(&opts);

	nosqlb_args(&funcs, &opts, argc, argv);
	if (!opts.std && !opts.std_memcache && !opts.tests_count) {
		nosqlb_usage(&opts, argv[0]);
		return 1;
	}
	opts.per = (opts.full) ? opts.count : (opts.count / opts.threads);

	printf("NoSQL benchmarking.\n\n");

	struct nosqlb bench;
	nosqlb_init(&bench, &funcs, &opts);

	nosqlb_run(&bench);
	nosqlb_free(&bench);

	nosqlb_func_free(&funcs);
	nosqlb_opt_free(&opts);
	return 0;
}
