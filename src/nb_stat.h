#ifndef NB_STAT_H_INCLUDED
#define NB_STAT_H_INCLUDED

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

#include <pthread.h>
#include <assert.h>

struct nb_stat_avg {
	int ps_read;
	int ps_write;
	int ps_req;
	int cnt_miss;

	int workers;
	int time;
	struct nb_stat_avg *next;
};


struct nb_stat {
	int cnt_write;
	int cnt_read;
	int cnt_miss;
	long long epoch;
	long long time_last_upd;
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
	int missed;
};

struct nb_history {
	int Smax, Scurrent;
	struct nb_stat *S;
	struct nb_stat_avg Savg;
	long long epoch;
	unsigned long long tm_start, tm_diff;

};

struct nb_statistics {
	struct nb_stat_avg *stats;
	int count;
	struct nb_stat_avg *current;
	struct nb_stat_final final;
	struct nb_stat_avg *head, *tail;
	int count_report;
	pthread_mutex_t lock_stats;
};

void nb_statistics_init(struct nb_statistics *s, int size);
void nb_statistics_free(struct nb_statistics *s);
void nb_statistics_resize(struct nb_statistics *s, int count);

static inline struct nb_stat_avg*
nb_statistics_for(struct nb_statistics *s, int pos) {
	assert(pos < s->count);
	return &s->stats[pos];
}

static inline void
nb_statistics_set(struct nb_statistics *s, int pos, struct nb_stat_avg *v) {
	memcpy(nb_statistics_for(s, pos), v, sizeof(struct nb_stat));
}

void nb_statistics_report(struct nb_statistics *s, int workers, int tick);
void nb_statistics_final(struct nb_statistics *s);
double nb_statistics_sum(struct nb_statistics *s);
int nb_statistics_csv(struct nb_statistics *s, char *file);

void nb_history_init(struct nb_history *s, int max);
void nb_history_free(struct nb_history *s);

unsigned long long nb_history_time(void);

void nb_history_avg(struct nb_history *s);

enum history_event_type {
	RT_READ,
	RT_WRITE,
	RT_MISS,
};

void
nb_history_add(struct nb_history *s, enum history_event_type e);

#endif
