
/*
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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

#include "nosqlbench.h"

extern struct nb nb;

enum nb_config_index_type {
	NB_CONFIG_NONE,
	NB_CONFIG_INT,
	NB_CONFIG_STRING
};

struct nb_config_index {
	int token;
	enum nb_config_index_type type;
	int *vi;
	char **vs;
};

struct nb_config {
	struct tnt_lex lex;
	char *file;
};

enum {
	NB_TK_CONFIGURATION = TNT_TK_CUSTOM,
	NB_TK_BENCHMARK,
	NB_TK_TIME_LIMIT,
	NB_TK_REQUEST_COUNT,
	NB_TK_REQUEST_BATCH_COUNT,
	NB_TK_REPORT_INTERVAL,
	NB_TK_REPORT_TYPE,
	NB_TK_CSV_FILE,
	NB_TK_CLIENT_HISTORY,
	NB_TK_CLIENT_CREATION_POLICY,
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

#define NB_DECLARE_KEYWORD_DEF(NAME, ID) { NAME, sizeof(NAME) - 1, ID }
#define NB_DECLARE_KEYWORD(NAME, ID) { NAME, sizeof(NAME) - 1, ID }
#define NB_DECLARE_KEYWORD_END() { 0, 0, 0 }

static struct tnt_lex_keyword nb_lex_keywords[] =
{
	NB_DECLARE_KEYWORD("configuration", NB_TK_CONFIGURATION),
	NB_DECLARE_KEYWORD("benchmark", NB_TK_BENCHMARK),
	NB_DECLARE_KEYWORD("time_limit", NB_TK_TIME_LIMIT),
	NB_DECLARE_KEYWORD("request_count", NB_TK_REQUEST_COUNT),
	NB_DECLARE_KEYWORD("request_batch_count", NB_TK_REQUEST_BATCH_COUNT),
	NB_DECLARE_KEYWORD("report_interval", NB_TK_REPORT_INTERVAL),
	NB_DECLARE_KEYWORD("report_type", NB_TK_REPORT_TYPE),
	NB_DECLARE_KEYWORD("csv_file", NB_TK_CSV_FILE),
	NB_DECLARE_KEYWORD("client_history", NB_TK_CLIENT_HISTORY),
	NB_DECLARE_KEYWORD("client_creation_policy", NB_TK_CLIENT_CREATION_POLICY),
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
	NB_DECLARE_KEYWORD_END()
};

#define NB_DECLARE_OPT(ID, TYPE, I, S) { ID, TYPE, I, S }
#define NB_DECLARE_OPT_INT(ID, VALUE) \
	NB_DECLARE_OPT(ID, NB_CONFIG_INT, VALUE, NULL)
#define NB_DECLARE_OPT_STR(ID, VALUE) \
	NB_DECLARE_OPT(ID, NB_CONFIG_STRING, NULL, VALUE)
#define NB_DECLARE_OPT_END() \
	NB_DECLARE_OPT(0, NB_CONFIG_NONE, NULL, NULL)

struct nb_config_index nb_config_index[] =
{
	NB_DECLARE_OPT_STR(NB_TK_BENCHMARK, &nb.opts.benchmark_policy_name),
	NB_DECLARE_OPT_INT(NB_TK_TIME_LIMIT, &nb.opts.time_limit),
	NB_DECLARE_OPT_INT(NB_TK_REQUEST_COUNT, &nb.opts.request_count),
	NB_DECLARE_OPT_INT(NB_TK_REQUEST_BATCH_COUNT, &nb.opts.request_batch_count),
	NB_DECLARE_OPT_INT(NB_TK_REPORT_INTERVAL, &nb.opts.report_interval),
	NB_DECLARE_OPT_STR(NB_TK_REPORT_TYPE, &nb.opts.report),
	NB_DECLARE_OPT_STR(NB_TK_CSV_FILE, &nb.opts.csv_file),
	NB_DECLARE_OPT_INT(NB_TK_CLIENT_HISTORY, &nb.opts.history_per_batch),
	NB_DECLARE_OPT_STR(NB_TK_CLIENT_CREATION_POLICY, &nb.opts.threads_policy_name),
	NB_DECLARE_OPT_INT(NB_TK_CLIENT_CREATION_INTERVAL, &nb.opts.threads_interval),
	NB_DECLARE_OPT_INT(NB_TK_CLIENT_CREATION_INCREMENT, &nb.opts.threads_increment),
	NB_DECLARE_OPT_INT(NB_TK_CLIENT_START, &nb.opts.threads_start),
	NB_DECLARE_OPT_INT(NB_TK_CLIENT_MAX, &nb.opts.threads_max),
	NB_DECLARE_OPT_STR(NB_TK_DB_DRIVER, &nb.opts.db),
	NB_DECLARE_OPT_STR(NB_TK_KEY_DISTRIBUTION, &nb.opts.key_dist),
	NB_DECLARE_OPT_INT(NB_TK_KEY_DISTRIBUTION_ITER, &nb.opts.key_dist_iter),
	NB_DECLARE_OPT_STR(NB_TK_KEY_TYPE, &nb.opts.key),
	NB_DECLARE_OPT_INT(NB_TK_VALUE_SIZE, &nb.opts.value_size),
	NB_DECLARE_OPT_INT(NB_TK_REPLACE, &nb.opts.dist_replace),
	NB_DECLARE_OPT_INT(NB_TK_UPDATE, &nb.opts.dist_update),
	NB_DECLARE_OPT_INT(NB_TK_DELETE, &nb.opts.dist_delete),
	NB_DECLARE_OPT_INT(NB_TK_SELECT, &nb.opts.dist_select),
	NB_DECLARE_OPT_STR(NB_TK_SERVER, &nb.opts.host),
	NB_DECLARE_OPT_INT(NB_TK_PORT, &nb.opts.port),
	NB_DECLARE_OPT_END()
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

static int nb_config_read(struct nb_config *cfg, struct tnt_tk *tk)
{
	struct nb_config_index *it = &nb_config_index[0];
	for (; it->type != NB_CONFIG_NONE; it++) {
		if (it->token != tk->tk)
			continue;
		switch (it->type) {
		case NB_CONFIG_INT:
			if (!nb_config_readint(cfg, it->vi) == -1)
				return -1;
			break;
		case NB_CONFIG_STRING:
			if (!nb_config_readsz(cfg, it->vs) == -1)
				return -1;
			break;
		case NB_CONFIG_NONE:
			break;
		}
		return 0;
	}
	return -1;
}

static int nb_config_process(struct nb_config *cfg)
{
	struct tnt_tk *tk;
	if (nb_config_expect(cfg, NB_TK_CONFIGURATION, NULL) == -1)
		return -1;
	if (nb_config_expect(cfg, '{', NULL) == -1)
		return -1;
	int eoc = 0;
	while (!eoc) {
		tnt_lex(&cfg->lex, &tk);
		if (nb_config_read(cfg, tk) == 0)
			continue;
		if (tk->tk == TNT_TK_PUNCT && tk->v.i32 == '}')
			eoc = 1;
		else
			return nb_config_error(cfg, tk, "unknown option");
	}
	return 0;
}

static int nb_config_readfile(char *file, char **buf, size_t *size)
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
	ssize_t off = 0;
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

int nb_config_parse(char *file)
{
	char *buf = NULL;
	size_t size;

	if (nb_config_readfile(file, &buf, &size) == -1)
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
	int rc = nb_config_process(&cfg);

	tnt_lex_free(&cfg.lex);
	return rc;
}
