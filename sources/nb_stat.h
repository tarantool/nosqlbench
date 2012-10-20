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
