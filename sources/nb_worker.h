#ifndef NB_WORKER_H_INCLUDED
#define NB_WORKER_H_INCLUDED

struct nb_worker {
	int id;
	struct nb_db db;
	struct nb_key_if *key;
	struct nb_key keyv;
	struct nb_workload workload;
	struct nb_history history;
	pthread_t tid;
	struct nb_worker *next;
};

struct nb_workers {
	struct nb_worker *head, *tail;
	volatile int count;
};

void nb_workers_init(struct nb_workers *workers);
void nb_workers_free(struct nb_workers *workers);

struct nb_worker*
nb_workers_create(struct nb_workers *workers, struct nb_db_if *dif,
		  struct nb_key_if *kif,
		  struct nb_key_distribution_if *distif,
		  struct nb_workload *workload, int history_max,
		  void *(*cb)(void *));

void nb_workers_join(struct nb_workers *workers);

#endif
