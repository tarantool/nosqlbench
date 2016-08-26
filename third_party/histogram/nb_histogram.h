#ifndef NB_HISTOGRAM_H_INCLUDED
#define NB_HISTOGRAM_H_INCLUDED

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

#include <stdio.h>

extern const size_t NB_HISTOGRAM_BUCKETS_COUNT;

struct nb_histogram {
	double min;
	double max;
	double sum;
	size_t size;
	size_t *buckets;
};

struct nb_histogram *
nb_histogram_new(void);

void
nb_histogram_merge(struct nb_histogram *dest, struct nb_histogram *src);

void
nb_histogram_delete(struct nb_histogram *hist);

void
nb_histogram_add(struct nb_histogram *hist, double val);

void
nb_histogram_clear(struct nb_histogram *hist);

double
nb_histogram_percentile(const struct nb_histogram *hist, double p);

void
nb_histogram_dump(const struct nb_histogram *hist, const char *interval_units,
		  double *percentiles, size_t percentiles_size);

#endif /* NB_HISTOGRAM_H_INCLUDED */
