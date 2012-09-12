
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <tarantool/tnt.h>
#include <tarantool/tnt_queue.h>
#include <tarantool/tnt_utf8.h>
#include <tarantool/tnt_lex.h>

#include "nb_alloc.h"
#include "nb_opt.h"

struct nb_config {
	struct tnt_lex lex;
	char *file;
};

enum {
	NB_TK_CONFIGURATION = TNT_TK_CUSTOM,
	NB_TK_BENCHMARK,
	NB_TK_NO_LIMIT,
	NB_TK_TIME_LIMIT,
	NB_TK_THREAD_LIMIT,
	NB_TK_REQUEST_COUNT,
	NB_TK_REQUEST_BATCH_COUNT,
	NB_TK_REPORT_INTERVAL,
	NB_TK_REPORT_TYPE,
	NB_TK_CSV_FILE,
	NB_TK_CLIENT_HISTORY,
	NB_TK_CLIENT_CREATION_POLICY,
	NB_TK_AT_ONCE,
	NB_TK_INTERVAL,
	NB_TK_CLIENT_CREATION_INTERVAL,
	NB_TK_CLIENT_CREATION_INCREMENT,
	NB_TK_CLIENT_START,
	NB_TK_CLIENT_MAX,
	NB_TK_DB_DRIVER,
	NB_TK_KEY_DISTRIBUTION,
	NB_TK_KEY_DISTRIBUTION_ITER,
	NB_TK_KEY_TYPE,
	NB_TK_VALUE_SIZE,
	NB_TK_REPLACE,
	NB_TK_UPDATE,
	NB_TK_DELETE,
	NB_TK_SELECT,
	NB_TK_SERVER,
	NB_TK_PORT
};

#define NB_DECLARE_KEYWORD(NAME, ID) { NAME, sizeof(NAME) - 1, ID }

static struct tnt_lex_keyword nb_lex_keywords[] =
{
	NB_DECLARE_KEYWORD("configuration", NB_TK_CONFIGURATION),
	NB_DECLARE_KEYWORD("benchmark", NB_TK_BENCHMARK),
	NB_DECLARE_KEYWORD("no_limit", NB_TK_NO_LIMIT),
	NB_DECLARE_KEYWORD("time_limit", NB_TK_TIME_LIMIT),
	NB_DECLARE_KEYWORD("thread_limit", NB_TK_THREAD_LIMIT),
	NB_DECLARE_KEYWORD("request_count", NB_TK_REQUEST_COUNT),
	NB_DECLARE_KEYWORD("request_batch_count", NB_TK_REQUEST_BATCH_COUNT),
	NB_DECLARE_KEYWORD("report_interval", NB_TK_REPORT_INTERVAL),
	NB_DECLARE_KEYWORD("report_type", NB_TK_REPORT_TYPE),
	NB_DECLARE_KEYWORD("csv_file", NB_TK_CSV_FILE),
	NB_DECLARE_KEYWORD("client_history", NB_TK_CLIENT_HISTORY),
	NB_DECLARE_KEYWORD("client_creation_policy", NB_TK_CLIENT_CREATION_POLICY),
	NB_DECLARE_KEYWORD("at_once", NB_TK_AT_ONCE),
	NB_DECLARE_KEYWORD("interval", NB_TK_INTERVAL),
	NB_DECLARE_KEYWORD("client_creation_interval", NB_TK_CLIENT_CREATION_INTERVAL),
	NB_DECLARE_KEYWORD("client_creation_increment", NB_TK_CLIENT_CREATION_INCREMENT),
	NB_DECLARE_KEYWORD("client_start", NB_TK_CLIENT_START),
	NB_DECLARE_KEYWORD("client_max", NB_TK_CLIENT_MAX),
	NB_DECLARE_KEYWORD("db_driver", NB_TK_DB_DRIVER),
	NB_DECLARE_KEYWORD("key_distribution", NB_TK_KEY_DISTRIBUTION),
	NB_DECLARE_KEYWORD("key_distribution_iter", NB_TK_KEY_DISTRIBUTION_ITER),
	NB_DECLARE_KEYWORD("key_type", NB_TK_KEY_TYPE),
	NB_DECLARE_KEYWORD("value_size", NB_TK_VALUE_SIZE),
	NB_DECLARE_KEYWORD("test_replace", NB_TK_REPLACE),
	NB_DECLARE_KEYWORD("test_update", NB_TK_UPDATE),
	NB_DECLARE_KEYWORD("test_delete", NB_TK_DELETE),
	NB_DECLARE_KEYWORD("test_select", NB_TK_SELECT),
	NB_DECLARE_KEYWORD("server", NB_TK_SERVER),
	NB_DECLARE_KEYWORD("port", NB_TK_PORT),
	{ NULL, 0, 0 }
};

static int
nb_config_error(struct nb_config *cfg, struct tnt_tk *last, char *fmt, ...)
{
	char msgu[256];
	va_list args;
	va_start(args, fmt);
	vsnprintf(msgu, sizeof(msgu), fmt, args);
	va_end(args);
	int line = (last) ? last->line : cfg->lex.line;
	int col = (last) ? last->col : cfg->lex.col;
	printf("%s %d:%d %s\n", cfg->file, line, col, msgu);
	return TNT_TK_ERROR;
}

static int
nb_config_expect(struct nb_config *cfg, int tk, struct tnt_tk **tkp)
{
	struct tnt_tk *tkp_ = NULL;
	int tk_ = tnt_lex(&cfg->lex, &tkp_);
	if (tk_ == TNT_TK_ERROR)
		return nb_config_error(cfg, NULL, "%s", cfg->lex.error);
	if (tk_ != tk) {
		if (tk < 0xff && ispunct(tk))
			return nb_config_error(cfg, tkp_, "expected '%c'", tk);
		return nb_config_error(cfg, tkp_, "expected '%s'",
				       tnt_lex_nameof(&cfg->lex, tk));
	}
	if (tkp)
		*tkp = tkp_;
	return 0;
}

static int
nb_config_expect_of(struct nb_config *cfg, struct tnt_tk **tkp, int argc, ...)
{
	va_list args;
	va_start(args, argc);

	char list[256];
	int off = 0;
	struct tnt_tk *tkp_ = NULL;
	int tk_ = tnt_lex(&cfg->lex, &tkp_);
	int i = argc;
	while (i-- > 0) {
		int tk = va_arg(&args, int);
		if (tk == tk_) {
			if (tkp)
				*tkp = tkp_;
			va_end(args);
			return tk;
		}
		off += snprintf(list + off, sizeof(list) - off, "%s%s", 
			        tnt_lex_nameof(&cfg->lex, tk),
				(i - 1 > 0) ? ", " : "");
	}
	return nb_config_error(cfg, tkp_, "expected any of '%s'", list);
}

static int nb_config_readint(struct nb_config *cfg, int *v) {
	struct tnt_tk *tk;
	if (nb_config_expect(cfg, TNT_TK_NUM32, &tk) == -1)
		return -1;
	*v = TNT_TK_I32(tk);
	return 0;
}

static int nb_config_readsz(struct nb_config *cfg, char **v) {
	struct tnt_tk *tk;
	if (nb_config_expect(cfg, TNT_TK_STRING, &tk) == -1)
		return -1;
	if (*v)
		free(*v);
	*v = nb_malloc(TNT_TK_S(tk)->size + 1);
	memcpy(*v, (char*)TNT_TK_S(tk)->data, TNT_TK_S(tk)->size + 1);
	return 0;
}

static int nb_config_process(struct nb_config *cfg, struct nb_options *opts)
{
	struct tnt_tk *tk;
	if (nb_config_expect(cfg, NB_TK_CONFIGURATION, NULL) == -1)
		return -1;
	if (nb_config_expect(cfg, '{', NULL) == -1)
		return -1;
	int eoc = 0;
	while (!eoc) {
		switch (tnt_lex(&cfg->lex, &tk)) {
		case NB_TK_BENCHMARK:
			switch (nb_config_expect_of(cfg, NULL, 3,
						    NB_TK_NO_LIMIT,
						    NB_TK_TIME_LIMIT,
						    NB_TK_THREAD_LIMIT)) {
			case NB_TK_NO_LIMIT:
				opts->benchmark_policy = NB_BENCHMARK_NOLIMIT;
				break;
			case NB_TK_TIME_LIMIT:
				opts->benchmark_policy = NB_BENCHMARK_TIMELIMIT;
				break;
			case NB_TK_THREAD_LIMIT:
				opts->benchmark_policy = NB_BENCHMARK_THREADLIMIT;
				break;
			default: return -1;
			}
			break;
		case NB_TK_TIME_LIMIT:
			if (!nb_config_readint(cfg, &opts->time_limit) == -1)
				return -1;
			break;
		case NB_TK_REQUEST_COUNT:
			if (!nb_config_readint(cfg, &opts->request_count) == -1)
				return -1;
			break;
		case NB_TK_REQUEST_BATCH_COUNT:
			if (!nb_config_readint(cfg, &opts->request_batch_count) == -1)
				return -1;
			break;
		case NB_TK_REPORT_INTERVAL:
			if (!nb_config_readint(cfg, &opts->report_interval) == -1)
				return -1;
			break;
		case NB_TK_REPORT_TYPE:
			if (!nb_config_readsz(cfg, &opts->report) == -1)
				return -1;
			break;
		case NB_TK_CSV_FILE:
			if (!nb_config_readsz(cfg, &opts->csv_file) == -1)
				return -1;
			break;
		case NB_TK_CLIENT_HISTORY:
			if (!nb_config_readint(cfg, &opts->history_per_batch) == -1)
				return -1;
			break;
		case NB_TK_CLIENT_CREATION_POLICY:
			switch (nb_config_expect_of(cfg, NULL, 2,
						    NB_TK_AT_ONCE,
						    NB_TK_INTERVAL)) {
			case NB_TK_AT_ONCE:
				opts->threads_policy = NB_THREADS_ATONCE;
				break;
			case NB_TK_INTERVAL:
				opts->threads_policy = NB_THREADS_INTERVAL;
				break;
			default: return -1;
			}
			break;
		case NB_TK_CLIENT_CREATION_INTERVAL:
			if (!nb_config_readint(cfg, &opts->threads_interval) == -1)
				return -1;
			break;
		case NB_TK_CLIENT_CREATION_INCREMENT:
			if (!nb_config_readint(cfg, &opts->threads_increment) == -1)
				return -1;
			break;
		case NB_TK_CLIENT_START:
			if (!nb_config_readint(cfg, &opts->threads_start) == -1)
				return -1;
			break;
		case NB_TK_CLIENT_MAX:
			if (!nb_config_readint(cfg, &opts->threads_max) == -1)
				return -1;
			break;
		case NB_TK_KEY_DISTRIBUTION:
			if (!nb_config_readsz(cfg, &opts->key_dist) == -1)
				return -1;
			break;
		case NB_TK_KEY_DISTRIBUTION_ITER:
			if (!nb_config_readint(cfg, &opts->key_dist_iter) == -1)
				return -1;
			break;
		case NB_TK_KEY_TYPE:
			if (!nb_config_readsz(cfg, &opts->key) == -1)
				return -1;
			break;
		case NB_TK_DB_DRIVER:
			if (!nb_config_readsz(cfg, &opts->db) == -1)
				return -1;
			break;
		case NB_TK_VALUE_SIZE:
			if (!nb_config_readint(cfg, &opts->value_size) == -1)
				return -1;
			break;
		case NB_TK_REPLACE:
			if (!nb_config_readint(cfg, &opts->dist_replace) == -1)
				return -1;
			break;
		case NB_TK_UPDATE:
			if (!nb_config_readint(cfg, &opts->dist_update) == -1)
				return -1;
			break;
		case NB_TK_DELETE:
			if (!nb_config_readint(cfg, &opts->dist_delete) == -1)
				return -1;
			break;
		case NB_TK_SELECT:
			if (!nb_config_readint(cfg, &opts->dist_select) == -1)
				return -1;
			break;
		case NB_TK_SERVER:
			if (!nb_config_readsz(cfg, &opts->host) == -1)
				return -1;
			break;
		case NB_TK_PORT:
			if (!nb_config_readint(cfg, &opts->port) == -1)
				return -1;
			break;
		default:
			if (tk->tk == TNT_TK_PUNCT && tk->v.i32 == '}')
				eoc = 1;
			else
				return nb_config_error(cfg, tk, "unknown option");
			break;
		}
	}
	return 0;
}

static int nb_config_read(char *file, char **buf, size_t *size)
{
	struct stat st;
	if (stat(file, &st) == -1) {
		printf("error: config file '%s': %s\n", file, strerror(errno));
		return -1;
	}
	int fd = open(file, O_RDONLY);
	if (fd == -1)
		return -1;
	*buf = nb_malloc(st.st_size);
	size_t off = 0;
	do {
		ssize_t r = read(fd, *buf + off, st.st_size - off);
		if (r == -1) {
			close(fd);
			free(*buf);
			printf("error: read(): %s\n", strerror(errno));
			return -1;
		}
		off += r;
	} while (off != st.st_size);
	*size = st.st_size;
	close(fd);
	return 0;
}

int nb_config_parse(struct nb_options *opts, char *file)
{
	char *buf = NULL;
	size_t size;

	if (nb_config_read(file, &buf, &size) == -1)
		return -1;

	struct nb_config cfg;
	memset(&cfg, 0, sizeof(cfg));

	if (tnt_lex_init(&cfg.lex, nb_lex_keywords, (unsigned char*)buf,
			 size) == -1) {
		free(buf);
		return -1;
	}
	free(buf);

	cfg.file = file;
	int rc = nb_config_process(&cfg, opts);

	tnt_lex_free(&cfg.lex);
	return rc;
}
