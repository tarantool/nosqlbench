
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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <pthread.h>

#include "nosqlbench.h"

extern struct nb nb;

static void nb_report_start(void)
{
	printf("NoSQL Benchmark.\n");
	printf("\n");
	printf("Server is %s:%d.\n",
	       nb.opts.host,
	       nb.opts.port);
	printf("\n");
}

static void nb_report_default_progress(int processed, int max)
{
	if (processed == 0 && max == 0) {
		printf("\n\n");
		return;
	}
	float percent = processed * 100.0 / max;
	int cols = 40;
	int done = percent * cols / 100.0;
	int left = cols - done;
	printf("Warmup [");
	while (done-- >= 0)
		printf(".");
	while (left-- > 0)
		printf(" ");
	printf("] %.2f%%\r", percent);
	fflush(NULL);
}

static void nb_report_default(void)
{
	printf("[%3d sec] [%2d threads] %7d req/s %7d read/s %7d write/s\n",
	       nb.tick,
	       nb.workers.count,
	       nb.stats.current->ps_req,
	       nb.stats.current->ps_read,
	       nb.stats.current->ps_write);
}

static void nb_report_default_final(void)
{
	char *report = 
	"\n"
	".----------.---------------.---------------.---------------.\n"
	"|   type   |    minimal    |    average    |     maximum   |\n"
	".----------.---------------.---------------.---------------.\n"
	"| read/s   |    %7d    |    %7d    |    %8d   |\n"
	"| write/s  |    %7d    |    %7d    |    %8d   |\n"
	"| req/s    |    %7d    |    %7d    |    %8d   |\n"
	"'----------.---------------.---------------.---------------'\n"
	"\n";
	printf(report, 
	       nb.stats.final.ps_read_min,
	       nb.stats.final.ps_read_avg,
	       nb.stats.final.ps_read_max,
	       nb.stats.final.ps_write_min,
	       nb.stats.final.ps_write_avg,
	       nb.stats.final.ps_write_max,
	       nb.stats.final.ps_req_min,
	       nb.stats.final.ps_req_avg,
	       nb.stats.final.ps_req_max);
}

static void nb_report_integral(void)
{
	printf("%.2f\n", nb_statistics_sum(&nb.stats));
}

struct nb_report_if nb_reports[] =
{
	{
		.name = "default",
		.init = NULL,
		.free = NULL,
		.report_start = nb_report_start,
		.report = nb_report_default,
		.progress = nb_report_default_progress,
		.report_final = nb_report_default_final
	},
	{
		.name = "integral_sum_only",
		.init = NULL,
		.free = NULL,
		.report_start = NULL,
		.report = NULL,
		.progress = NULL,
		.report_final = nb_report_integral
	},
	{
		.name = NULL
	}
};

struct nb_report_if *nb_report_match(const char *name)
{
	int i = 0;
	while (nb_reports[i].name) {
		if (strcmp(nb_reports[i].name, name) == 0)
			return &nb_reports[i];
		i++;
	}
	return NULL;
}
