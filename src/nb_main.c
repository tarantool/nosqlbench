
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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <tarantool/tnt.h>

#include <nb_queue.h>
#include <nb_arg.h>
#include <nb_stat.h>
#include <nb_func.h>
#include <nb_test.h>
#include <nb_opt.h>
#include <nb_cb.h>
#include <nb.h>
#include <nb_data.h>
#include <nb_plot.h>

static char*
nb_usage_onoff(int val) {
	return (val) ? "on" : "off";
}

static void
nb_usage(struct nb_opt *opts, char *name)
{
	printf("%s [options]\n\n", name);

	printf("NoSQL benchmarking.\n");
	printf("connection:\n");
	printf("  -a, --server-host [host]             server address (%s)\n", opts->host);
	printf("  -p, --server-port [port]             server port (%d)\n", opts->port);
	printf("  -r, --buf-recv [rbuf]                receive buffer size (%d)\n", opts->rbuf);
	printf("  -s, --buf-send [sbuf]                send buffer size (%d)\n", opts->sbuf);
	printf("  -t, --threads [count]                connection threads (%d)\n\n", opts->threads);

	printf("benchmark:\n");
	printf("  -L, --test-list                      list available tests\n");
	printf("  -A, --test-std-tnt                   standard tarantool testing set (%s)\n", nb_usage_onoff(opts->std));
	printf("  -M, --test-std-mc                    standard memcache testing set (%s)\n", nb_usage_onoff(opts->std_memcache));
	printf("  -T, --test [name,...]                list of tests\n");
	printf("  -B, --test-buf [buf,...]             test buffer sizes\n");
	printf("  -F, --test-buf-file [path]           read tests buffer sizes from file\n");
	printf("  -Z, --test-buf-zone [fmt]            set tests buffer zone, eg. 5-100:5\n");
	printf("  -U, --test-full                      do full request range per thread (%s)\n", nb_usage_onoff(opts->full));
	printf("  -C, --count [count]                  request count (%d)\n", opts->count);
	printf("  -R, --repeat [count]                 request repeat (%d)\n\n", opts->rep);

	printf("statistics:\n");
	printf("  -X, --stat-esync                     display stat ex. threads sync overhead (%s)\n", nb_usage_onoff(opts->esync));
	printf("  -P, --stat-plot [path]               generate gnuplot files\n");
	printf("  -D, --stat-plot-load [path] [file]   generate gnuplot files from statistic file\n");
	printf("  -S, --stat-save [path]               save statistics to file\n");
	printf("  -I, --stat-info [path]               display statistic file \n");
	printf("  -G, --stat-info-int [path]           display statistic file integral values\n\n");

	printf("other:\n");
	printf("  -h, --help                           show usage\n\n");

	printf("examples:\n");
	printf("  # standard iproto benchmark (insert, update, select)\n");
	printf("  nb --test-std-tnt\n\n");

	printf("  # benchmark insert, select for range of 5 - 100 bytes payload\n");
	printf("  # with increment 5 for 200k operations in 10 threads\n");
	printf("  nb -T tnt-insert,tnt-select -Z 5-100:5 -C 200000 -R 3 -t 10\n\n");

	printf("  # benchmark memcache protocol for 32, 64, 128 bytes payload\n");
	printf("  # with plot generation\n");
	printf("  nb -T memcache-set -B 32,64,128 -C 10000 -P\n");

	exit(1);
}

static
struct nb_arg_cmd cmds[] =
{
	{ "-h",              0, NB_ARG_HELP          },
	{ "--help",          0, NB_ARG_HELP          },
	{ "-a",              1, NB_ARG_SERVER_HOST   },
	{ "--server-host",   1, NB_ARG_SERVER_HOST   },
	{ "-p",              1, NB_ARG_SERVER_PORT   },
	{ "--server-port",   1, NB_ARG_SERVER_PORT   },
	{ "-t",              1, NB_ARG_THREADS       },
	{ "--threads",       1, NB_ARG_THREADS       },
	{ "-r",              1, NB_ARG_BUF_RECV      },
	{ "--buf-recv",      1, NB_ARG_BUF_RECV      },
	{ "-s",              1, NB_ARG_BUF_SEND      },
	{ "--buf-send",      1, NB_ARG_BUF_SEND      },
	{ "-M",              0, NB_ARG_TEST_STD_MC   },
	{ "--test-std-mc",   0, NB_ARG_TEST_STD_MC   },
	{ "-A",              0, NB_ARG_TEST_STD      },
	{ "--test-std-tnt",  0, NB_ARG_TEST_STD      },
	{ "-T",              1, NB_ARG_TEST          },
	{ "--test",          1, NB_ARG_TEST          },
	{ "-B",              1, NB_ARG_TEST_BUF      },
	{ "--test-buf",      1, NB_ARG_TEST_BUF      },
	{ "-F",              1, NB_ARG_TEST_BUF_FILE },
	{ "--test-buf-file", 1, NB_ARG_TEST_BUF_FILE },
	{ "-Z",              1, NB_ARG_TEST_BUF_ZONE },
	{ "--test-buf-zone", 1, NB_ARG_TEST_BUF_ZONE },
	{ "-L",              0, NB_ARG_TEST_LIST     },
	{ "--test-list",     0, NB_ARG_TEST_LIST     },
	{ "-C",              1, NB_ARG_COUNT         },
	{ "--count",         1, NB_ARG_COUNT         },
	{ "-R",              1, NB_ARG_REP           },
	{ "--repeat",        1, NB_ARG_REP           },
	{ "-U",              0, NB_ARG_FULL          },
	{ "--test-full",     0, NB_ARG_FULL          },
	{ "-X",              0, NB_ARG_ESYNC         },
	{ "--stat-esync",    0, NB_ARG_ESYNC         },
	{ "-P",              1, NB_ARG_PLOT          },
	{ "--stat-plot",     1, NB_ARG_PLOT          },
	{ "-D",              2, NB_ARG_PLOT_LOAD     },
	{ "--stat-plot-load",2, NB_ARG_PLOT_LOAD     },
	{ "-S",              1, NB_ARG_SAVE          },
	{ "--stat-save",     1, NB_ARG_SAVE          },
	{ "-I",              1, NB_ARG_INFO          },
	{ "--stat-info",     1, NB_ARG_INFO          },
	{ "-G",              1, NB_ARG_INFO_INT      },
	{ "--stat-info-int", 1, NB_ARG_INFO_INT      },
	{ NULL,              0, 0                    }
};

static void
nb_add_tests(struct nb_opt *opts, char *argp)
{
	char list[1024];
	strncpy(list, argp, sizeof(list));
	char *p;
	for (p = strtok(list, ",") ; p ; p = strtok(NULL, ",")) {
		struct nb_opt_test *test =
			malloc(sizeof(struct nb_opt_test));
		if (test == NULL)
			return;
		test->test = strdup(p);
		STAILQ_INSERT_TAIL(&opts->tests, test, next);
		opts->tests_count++;
	}
}

static void
nb_add_bufs(struct nb_opt *opts, char *argp)
{
	char buflist[1024];
	strncpy(buflist, argp, sizeof(buflist));
	char *p;
	for (p = strtok(buflist, ",") ; p ; p = strtok(NULL, ",")) {
		struct nb_opt_buf *buf =
			malloc(sizeof(struct nb_opt_buf));
		if (buf == NULL)
			return;
		buf->buf = atoi(p);
		STAILQ_INSERT_TAIL(&opts->bufs, buf, next);
		opts->bufs_count++;
	}
}

static void
nb_add_bufs_chop(char *buf, int size)
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
nb_add_bufs_file(struct nb_opt *opts, char *argp)
{
	FILE *f = fopen(argp, "r");
	if (f == NULL) {
		printf("failed to open file %s\n", argp);
		return;
	}
	char sz[64];
	while (fgets(sz, sizeof(sz), f)) {
		struct nb_opt_buf *buf =
			malloc(sizeof(struct nb_opt_buf));
		if (buf == NULL)
			return;
		nb_add_bufs_chop(sz, sizeof(sz));
		buf->buf = atoi(sz);
		STAILQ_INSERT_TAIL(&opts->bufs, buf, next);
		opts->bufs_count++;
	}
	fclose(f);
}

static void
nb_add_bufs_zone(struct nb_opt *opts, char *argp)
{
	/* start-end:increment */
	char *p;
	char sz[256];
	snprintf(sz, sizeof(sz), "%s", argp);

	int begin = 0,
	    end = 0,
	    inc = 0;
	int state = 0;
	for (p = strtok(sz, "-") ; p ; p = strtok(NULL, ":")) {
		switch (state) {
		case 0: begin = atoi(p);
			break;
		case 1: end = atoi(p);
			break;
		case 2: inc = atoi(p);
			break;
		default:
			goto error;
		}
		state++;
	}
	if (state != 3)
		goto error;

	int b = 0;
	for (b = begin ; b <= end ; b += inc) {
		struct nb_opt_buf *buf =
			malloc(sizeof(struct nb_opt_buf));
		if (buf == NULL)
			return;
		buf->buf = b;
		STAILQ_INSERT_TAIL(&opts->bufs, buf, next);
		opts->bufs_count++;
	}
	return;
error:
	printf("bad zone format\n");
}

static void
nb_args(struct nb_funcs * funcs,
	struct nb_opt *opts, int argc, char * argv[])
{
	struct nb_arg args;
	nb_arg_init(&args, cmds, argc, argv);

	while (1) {
		int ac;
		char **av;
		switch (nb_arg(&args, &ac, &av)) {
		case NB_ARG_DONE:
			return;
		case NB_ARG_UNKNOWN:
		case NB_ARG_ERROR:
		case NB_ARG_HELP:
			nb_usage(opts, argv[0]);
			break;
		case NB_ARG_SERVER_HOST:
			opts->host = av[0];
			break;
		case NB_ARG_SERVER_PORT:
			opts->port = atoi(av[0]);
			break;
		case NB_ARG_THREADS:
			opts->threads = atoi(av[0]);
			break;
		case NB_ARG_BUF_RECV:
			opts->rbuf = atoi(av[0]);
			break;
		case NB_ARG_BUF_SEND:
			opts->sbuf = atoi(av[0]);
			break;
		case NB_ARG_TEST_STD_MC:
			opts->std_memcache = 1;
			break;
		case NB_ARG_TEST_STD:
			opts->std = 1;
			break;
		case NB_ARG_TEST:
			nb_add_tests(opts, av[0]);
			break;
		case NB_ARG_TEST_BUF:
			nb_add_bufs(opts, av[0]);
			break;
		case NB_ARG_TEST_BUF_FILE:
			nb_add_bufs_file(opts, av[0]);
			break;
		case NB_ARG_TEST_BUF_ZONE:
			nb_add_bufs_zone(opts, av[0]);
			break;
		case NB_ARG_TEST_LIST:
			printf("available tests:\n");
			struct nb_func *func;
			STAILQ_FOREACH(func, &funcs->list, next)
				printf("  %s\n", func->name);
			exit(0);
			break;
		case NB_ARG_COUNT:
			opts->count = atoi(av[0]);
			break;
		case NB_ARG_REP:
			opts->rep = atoi(av[0]);
			break;
		case NB_ARG_FULL:
			opts->full = 1;
			break;
		case NB_ARG_ESYNC:
			opts->esync = 1;
			break;
		case NB_ARG_PLOT:
			opts->plot = av[0];
			break;
		case NB_ARG_PLOT_LOAD:
			opts->plot = av[0];
			opts->data = NB_OPT_DATA_PLOT;
			opts->datap= av[1];
			break;
		case NB_ARG_SAVE:
			opts->data = NB_OPT_DATA_SAVE;
			opts->datap = av[0];
			break;
		case NB_ARG_INFO:
			opts->data = NB_OPT_DATA_INFO;
			opts->datap = av[0];
			break;
		case NB_ARG_INFO_INT:
			opts->data = NB_OPT_DATA_INFO_INT;
			opts->datap = av[0];
			break;
		}
	}
}

int
main(int argc, char * argv[])
{
	struct nb_funcs funcs;
	nb_func_init(&funcs);
	nb_cb_init(&funcs);

	struct nb_opt opts;
	nb_opt_init(&opts);

	struct nb bench;
	nb_init(&bench, &funcs, &opts);
	nb_args(&funcs, &opts, argc, argv);

	/* data file operations */
	if (opts.data && opts.data != NB_OPT_DATA_SAVE) {
		if (nb_data_load(&bench) == -1) {
			printf("failed to load data file: %s\n", opts.datap);
			return 1;
		}
		switch (opts.data) {
		case NB_OPT_DATA_INFO:
			nb_info(&bench);
			break;
		case NB_OPT_DATA_INFO_INT:
			nb_info_int(&bench);
			break;
		case NB_OPT_DATA_PLOT:
			nb_plot_gen(&bench);
			break;
		}
		goto done;
	}

	/* benchmarking */
	if (!opts.std && !opts.std_memcache && !opts.tests_count) {
		nb_usage(&opts, argv[0]);
		return 1;
	}
	opts.per = (opts.full) ? opts.count : (opts.count / opts.threads);

	printf("NoSQL benchmarking.\n\n");
	nb_run(&bench);
done:
	nb_free(&bench);
	nb_func_free(&funcs);
	nb_opt_free(&opts);
	return 0;
}
