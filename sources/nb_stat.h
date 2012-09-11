#ifndef NB_STAT_H_INCLUDED
#define NB_STAT_H_INCLUDED

struct nb_stat {
	int ps_write;
	int ps_read;
	int ps_req;
	int cnt_write;
	int cnt_read;
	int cnt_miss;
	float latency_req;
	float latency_batch;

	int workers;
	int time;
	struct nb_stat *next;
};

struct nb_stat_final {
	int ps_read_min;
	int ps_read_max;
	int ps_read_avg;
	int ps_write_min;
	int ps_write_max;
	int ps_write_avg;
	int ps_req_min;
	int ps_req_max;
	int ps_req_avg;
	float latency_req_min;
	float latency_req_max;
	float latency_req_avg;
	int missed;
};

struct nb_history {
	int Smax, Stop, Scurrent;
	struct nb_stat *S;
	struct nb_stat Savg;
	unsigned long long tm_start, tm_diff;
};

struct nb_statistics {
	struct nb_stat *stats;
	int count;
	struct nb_stat *current;
	struct nb_stat_final final;
	struct nb_stat *head, *tail;
	int count_report;
};

void nb_statistics_init(struct nb_statistics *s, int size);
void nb_statistics_free(struct nb_statistics *s);
void nb_statistics_resize(struct nb_statistics *s, int count);

static inline struct nb_stat*
nb_statistics_for(struct nb_statistics *s, int pos) {
	assert(pos < s->count);
	return &s->stats[pos];
}

static inline void
nb_statistics_set(struct nb_statistics *s, int pos, struct nb_stat *v) {
	memcpy(nb_statistics_for(s, pos), v, sizeof(struct nb_stat));
}

void nb_statistics_report(struct nb_statistics *s, int workers, int tick);
void nb_statistics_final(struct nb_statistics *s);
int nb_statistics_csv(struct nb_statistics *s, char *file);

void nb_history_init(struct nb_history *s, int max);
void nb_history_free(struct nb_history *s);

unsigned long long nb_history_time(void);

static inline void
nb_history_start(struct nb_history *s) {
	s->tm_start = nb_history_time();
}

static inline void
nb_history_stop(struct nb_history *s) {
	s->tm_diff =  nb_history_time() - s->tm_start;
}

void nb_history_avg(struct nb_history *s);
void nb_history_account(struct nb_history *s, int batch);

static inline struct nb_stat*
nb_history_current(struct nb_history *s) {
	return &s->S[s->Scurrent];
}

static inline void
nb_history_add(struct nb_history *s, int is_ro, int count) {
	struct nb_stat *current = nb_history_current(s);
	if (is_ro)
		current->cnt_read += count;
	else
		current->cnt_write += count;
}

#endif
