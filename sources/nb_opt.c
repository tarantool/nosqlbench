
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
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "nb_alloc.h"
#include "nb_opt.h"

void nb_opt_init(struct nb_options *opts)
{
	opts->benchmark_policy = NB_BENCHMARK_NOLIMIT;
	opts->report_interval = 1;
	opts->time_limit = 0;
	opts->report = nb_strdup("default");
	opts->csv_file = NULL;
	opts->threads_policy = NB_THREADS_ATONCE;
	opts->threads_start = 10;
	opts->threads_max = 10;
	opts->threads_interval = 1;
	opts->threads_increment = 1;
	opts->request_count = 10000;
	opts->request_batch_count = 1000;
	opts->history_per_batch = 16;
	opts->db = nb_strdup("tarantool");
	opts->key = nb_strdup("string");
	opts->key_dist = nb_strdup("uniform");
	opts->key_dist_iter = 4;
	opts->value_size = 16;
	opts->dist_replace = 40;
	opts->dist_update = 10;
	opts->dist_delete = 10;
	opts->dist_select = 40;
	opts->host = nb_strdup("127.0.0.1");
	opts->port = 33013;
}

void nb_opt_free(struct nb_options *opts)
{
	free(opts->benchmark_policy_name);
	free(opts->threads_policy_name);
	free(opts->report);
	free(opts->csv_file);
	free(opts->db);
	free(opts->key);
	free(opts->key_dist);
	free(opts->host);
}
