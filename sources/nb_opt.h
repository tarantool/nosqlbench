#ifndef NB_OPT_H_INCLUDED
#define NB_OPT_H_INCLUDED

enum nb_policy_benchmark {
	NB_BENCHMARK_NOLIMIT,
	NB_BENCHMARK_TIMELIMIT,
	NB_BENCHMARK_REQLIMIT,
	NB_BENCHMARK_THREADLIMIT
};

enum nb_policy_threads {
	NB_THREADS_ATONCE,
	NB_THREADS_INTERVAL
};

struct nb_options {
	enum nb_policy_benchmark benchmark_policy;

	int time_limit;

	int report_interval;
	const char *report;

	enum nb_policy_threads threads_policy;
	int threads_start;
	int threads_max;
	int threads_increment;
	int threads_interval;

	int request_count;
	int request_batch_count;

	int history_per_batch;

	char *csv_file;

	const char *db;
	const char *key;
	const char *key_dist;
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
int nb_opt_configure(struct nb_options *opts, char *file);

#endif
