#ifndef NB_H_INCLUDED
#define NB_H_INCLUDED

struct nb {
	struct nb_options opts;
	struct nb_key_if *key;
	struct nb_key_distribution_if *key_dist;
	struct nb_db_if *db;
	struct nb_report_if *report;
	struct nb_workload workload;
	struct nb_workers workers;
	struct nb_statistics stats;
	volatile int is_done;
	int tick;
};

extern struct nb nb;

static inline int nb_period_equal(int period) {
	return nb.tick > 0 && nb.tick % period == 0;
}

static inline void nb_tick(void) {
	nb.tick++;
}

#endif
