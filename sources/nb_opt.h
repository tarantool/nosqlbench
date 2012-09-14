#ifndef NB_OPT_H_INCLUDED
#define NB_OPT_H_INCLUDED

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

enum nb_policy_benchmark {
	NB_BENCHMARK_NOLIMIT,
	NB_BENCHMARK_TIMELIMIT,
	NB_BENCHMARK_THREADLIMIT
};

enum nb_policy_threads {
	NB_THREADS_ATONCE,
	NB_THREADS_INTERVAL
};

struct nb_options {
	enum nb_policy_benchmark benchmark_policy;
	char *benchmark_policy_name;

	int time_limit;

	int report_interval;
	char *report;

	int request_count;
	int request_batch_count;
	int history_per_batch;

	enum nb_policy_threads threads_policy;
	char *threads_policy_name;
	int threads_start;
	int threads_max;
	int threads_increment;
	int threads_interval;

	char *csv_file;

	char *db;
	char *key;
	char *key_dist;
	int key_dist_iter;

	int value_size;
	
	int dist_replace;
	int dist_update;
	int dist_delete;
	int dist_select;

	char *host;
	int port;
};

void nb_opt_init(struct nb_options *opts);
void nb_opt_free(struct nb_options *opts);

#endif
