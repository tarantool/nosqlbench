
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
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/time.h>

#include "nb_alloc.h"
#include "nb_stat.h"

void nb_statistics_init(struct nb_statistics *s, int size)
{
	s->stats = NULL;
	s->count = 0;
	s->count_report = 0;
	s->head = NULL;
	s->tail = NULL;
	memset(&s->current, 0, sizeof(s->current));
	memset(&s->final, 0, sizeof(s->final));

	nb_statistics_resize(s, size);
}

void nb_statistics_free(struct nb_statistics *s)
{
	if (s->stats)
		free(s->stats);
	struct nb_stat_avg *c = s->head, *n;
	while (c) {
		n = c->next;
		free(c);
		c = n;
	}
}

void nb_statistics_resize(struct nb_statistics *s, int count)
{
	if (count < s->count)
		return;
	int bottom = s->count;
	s->count = count;
	s->stats = nb_realloc((void*)s->stats, count * sizeof(struct nb_stat));
	memset(s->stats + bottom, 0,
	      (s->count - bottom) * sizeof(struct nb_stat));
}

static void
nb_statistics_current(struct nb_statistics *s, struct nb_stat_avg *dest)
{
	memset(dest, 0, sizeof(struct nb_stat));
	int i = 0;
	for (; i < s->count; i++) {
		dest->ps_read += s->stats[i].ps_read;
		dest->ps_write += s->stats[i].ps_write;
		dest->ps_req += s->stats[i].ps_req;
		dest->cnt_miss += s->stats[i].cnt_miss;
	}
}

void nb_statistics_report(struct nb_statistics *s, int workers, int tick)
{
	struct nb_stat_avg *n = nb_malloc(sizeof(struct nb_stat_avg));
	nb_statistics_current(s, n);
	n->workers = workers;
	n->time = tick;
	n->next = NULL;
	if (s->head == NULL)
		s->head = n;
	else
		s->tail->next = n;
	s->tail = n;
	s->count_report++;
	s->current = n;
}

void nb_statistics_final(struct nb_statistics *s)
{
	memset(&s->final, 0, sizeof(s->final));
	struct nb_stat_avg *iter = s->head;
	if (iter == NULL)
		return;

	s->final.ps_req_min = iter->ps_req;
	s->final.ps_read_min = iter->ps_read;
	s->final.ps_write_min = iter->ps_write;

	int ps_req_sum = 0;
	int ps_read_sum = 0;
	int ps_write_sum = 0;

	while (iter) {
		if (iter->ps_req < s->final.ps_req_min)
			s->final.ps_req_min = iter->ps_req;
		if (iter->ps_req > s->final.ps_req_max)
			s->final.ps_req_max = iter->ps_req;

		if (iter->ps_write < s->final.ps_write_min)
			s->final.ps_write_min = iter->ps_write;
		if (iter->ps_write > s->final.ps_write_max)
			s->final.ps_write_max = iter->ps_write;

		if (iter->ps_read < s->final.ps_read_min)
			s->final.ps_read_min = iter->ps_read;
		if (iter->ps_read > s->final.ps_read_max)
			s->final.ps_read_max = iter->ps_read;

		ps_req_sum += iter->ps_req;
		ps_read_sum += iter->ps_read;
		ps_write_sum += iter->ps_write;
		iter = iter->next;
	}
	
	s->final.ps_req_avg = ps_req_sum / s->count_report;
	s->final.ps_read_avg = ps_read_sum / s->count_report;
	s->final.ps_write_avg = ps_write_sum / s->count_report;

	s->final.missed = s->tail->cnt_miss;
}

static int nb_statistics_min_workers(struct nb_statistics *s) {
	int min = s->head->workers;
	struct nb_stat_avg *n = s->head->next;
	while (n) {
		if (n->ps_req < min)
			min = n->workers;
		n = n->next;
	}
	return min;
}

static int nb_statistics_max_workers(struct nb_statistics *s) {
	int max = 0;
	struct nb_stat_avg *n = s->head;
	while (n) {
		if (n->ps_req > max)
			max = n->workers;
		n = n->next;
	}
	return max;
}

static double
nb_statistics_integrate(struct nb_statistics *s, int workers) {
	/* match lower bound */
	struct nb_stat_avg *start = NULL;
	struct nb_stat_avg *n = s->head;
	while (n) {
		int s = (start == NULL || n->workers >= start->workers);
		if (n->workers <= workers && s)
			start = n;
		n = n->next;
	}
	if (start == NULL || start->next == NULL)
		return 0.0;
	/* interpolate rps value */
	double rps = start->ps_req +
		((workers - start->workers) / (start->next->workers - start->workers)) *
			(start->next->ps_req - start->ps_req);
	return rps;
}

double nb_statistics_sum(struct nb_statistics *s) {
	double sum = 0.0;

	if (s->count_report <= 1)
		return 0.0;

	int a = nb_statistics_min_workers(s);
	int b = nb_statistics_max_workers(s);

	double acq = 1000;
	double interval = (b - a) / acq;
	int i;
	for (i = 1; i<= acq ; i++)
		sum += nb_statistics_integrate(s, a + interval * (i - 0.5));
	sum *= interval;

	return sum;
}

int nb_statistics_csv(struct nb_statistics *s, char *file)
{
	FILE *f = fopen(file, "w");
	if (f == NULL)
		return -1;
	struct nb_stat_avg *iter = s->head;
	while (iter) {
		int rc = fprintf(f, "%d, %d, %d, %d, %d\n",
				 iter->workers,
				 iter->time,
				 iter->ps_req,
				 iter->ps_read,
				 iter->ps_write);
		if (rc == -1)
			return -1;
		iter = iter->next;
	}
	fclose(f);
	return 0;
}

void nb_history_init(struct nb_history *s, int max)
{
	s->Smax = max;
	s->Scurrent = 0;
	s->S = nb_malloc(sizeof(struct nb_stat) * max);
	memset(s->S, 0, sizeof(struct nb_stat) * max);
}

void nb_history_free(struct nb_history *s)
{
	if (s->S)
		free(s->S);
}

unsigned long long nb_history_time(void)
{
	unsigned long long tm;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	tm = tv.tv_sec * 1000000ull;
	tm += tv.tv_usec;
	return tm;
}

void
nb_history_add(struct nb_history *s, enum history_event_type e, int count) {
	long long t = nb_history_time();
	long long epoch = t * s->Smax / 1000000;
	if (epoch > s->epoch) {
		long long e_start = s->epoch + 1;
		if (e_start < epoch - s->Smax)
			e_start = epoch - s->Smax;
		for (long long e = e_start; e <= epoch; e++) {
			long long pos = e % s->Smax;
			s->S[pos].cnt_read = 0;
			s->S[pos].cnt_write = 0;
			s->S[pos].cnt_miss = 0;
			s->S[pos].epoch = e;
			s->S[pos].time_last_upd = e * 1000000 / 16;
		}
		s->epoch = epoch;
	}
	long long pos = epoch % s->Smax;
	struct nb_stat *current = &s->S[pos];
	if (e == RT_READ)
		current->cnt_read += count;
	else if (e == RT_WRITE)
		current->cnt_write += count;
	else if (e == RT_MISS)
		current->cnt_miss += count;
	current->time_last_upd = t;
}

void nb_history_avg(struct nb_history *s)
{
	long long t = nb_history_time();
	long long epoch = t * s->Smax / 1000000;
	long long min_epoch = epoch - s->Smax + 1;
	memset(&s->Savg, 0, sizeof(s->Savg));
	int i = 0;
	double total_time = 0;
	int total_read = 0, total_write = 0, total_miss = 0;
	for (; i < s->Smax; i++) {
		if (s->S[i].epoch == epoch) {
			long long diff =
				s->S[i].time_last_upd - epoch * 1000000 / 16;
			if (diff == 0)
				continue;
			total_time += (double)diff / 1000000;
		} else if (s->S[i].epoch > epoch || s->S[i].epoch < min_epoch) {
			continue;
		} else {
			total_time += 1. / s->Smax;
		}
		total_read += s->S[i].cnt_read;
		total_write += s->S[i].cnt_write;
		total_miss += s->S[i].cnt_miss;
	}
	total_time = total_time == 0. ? 1. : total_time;
	s->Savg.ps_read = (int)(total_read / total_time);
	s->Savg.ps_write = (int)(total_write / total_time);
	(void)total_miss;
	s->Savg.ps_req = s->Savg.ps_read + s->Savg.ps_write;
}
