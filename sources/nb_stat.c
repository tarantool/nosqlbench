
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
	struct nb_stat *c = s->head, *n;
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
nb_statistics_current(struct nb_statistics *s, struct nb_stat *dest)
{
	memset(dest, 0, sizeof(struct nb_stat));
	int i = 0;
	for (; i < s->count; i++) {
		dest->ps_read += s->stats[i].ps_read;
		dest->ps_write += s->stats[i].ps_write;
		dest->ps_req += s->stats[i].ps_req;
		dest->latency_req += s->stats[i].latency_req;
		dest->latency_batch += s->stats[i].latency_batch;
		dest->cnt_miss += s->stats[i].cnt_miss;
	}
	dest->latency_req /= s->count;
	dest->latency_batch /= s->count;
}

void nb_statistics_report(struct nb_statistics *s, int workers, int tick)
{
	struct nb_stat *n = nb_malloc(sizeof(struct nb_stat));
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
	struct nb_stat *iter = s->head;
	if (iter == NULL)
		return;

	s->final.latency_req_min = iter->latency_req;
	s->final.ps_req_min = iter->ps_req;
	s->final.ps_read_min = iter->ps_read;
	s->final.ps_write_min = iter->ps_write;

	float latency_req_sum = 0.0;
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

		if (iter->latency_req < s->final.latency_req_min)
			s->final.latency_req_min = iter->latency_req;
		if (iter->latency_req > s->final.latency_req_max)
			s->final.latency_req_max = iter->latency_req;

		latency_req_sum += iter->latency_req;
		ps_req_sum += iter->ps_req;
		ps_read_sum += iter->ps_read;
		ps_write_sum += iter->ps_write;
		iter = iter->next;
	}
	
	s->final.ps_req_avg = ps_req_sum / s->count_report;
	s->final.ps_read_avg = ps_read_sum / s->count_report;
	s->final.ps_write_avg = ps_write_sum / s->count_report;
	s->final.latency_req_avg = latency_req_sum / s->count_report;

	s->final.missed = s->tail->cnt_miss;
}

int nb_statistics_csv(struct nb_statistics *s, char *file)
{
	FILE *f = fopen(file, "w");
	if (f == NULL)
		return -1;
	struct nb_stat *iter = s->head;
	while (iter) {
		int rc = fprintf(f, "%d, %d, %d, %d, %d, %f\n",
				 iter->workers,
				 iter->time,
				 iter->ps_req,
				 iter->ps_read,
				 iter->ps_write,
				 iter->latency_req);
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
	s->Stop = 1;
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
	tm = ((long)tv.tv_sec) * 1000;
	tm += tv.tv_usec / 1000;
	return tm;
}

void nb_history_avg(struct nb_history *s)
{
	memset(&s->Savg, 0, sizeof(s->Savg));
	int i = 0;
	for (; i < s->Stop; i++) {
		s->Savg.ps_read += s->S[i].ps_read;
		s->Savg.ps_write += s->S[i].ps_write;
		s->Savg.ps_req += s->S[i].ps_req;
		s->Savg.latency_req += s->S[i].latency_req;
		s->Savg.latency_batch += s->S[i].latency_batch;
		s->Savg.cnt_miss += s->S[i].cnt_miss;
	}
	s->Savg.ps_read /= s->Stop;
	s->Savg.ps_write /= s->Stop;
	s->Savg.ps_req /= s->Stop;
	s->Savg.latency_req /= s->Stop;
	s->Savg.latency_batch /= s->Stop;
}

void nb_history_account(struct nb_history *s, int batch)
{
	nb_history_stop(s);

	struct nb_stat *current = &s->S[s->Scurrent];

	current->latency_batch = s->tm_diff / 1000.0;
	current->latency_req = current->latency_batch / batch;
	current->ps_req = (current->cnt_read + current->cnt_write) /
		           current->latency_batch;
	current->ps_read = current->cnt_read / current->latency_batch;
	current->ps_write = current->cnt_write / current->latency_batch;
	current->cnt_read = 0;
	current->cnt_write = 0;

	s->Scurrent++;
	if (s->Stop < s->Smax)
		s->Stop++;
	if (s->Scurrent == s->Smax)
		s->Scurrent = 0;

	nb_history_start(s);
}
