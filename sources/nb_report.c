
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
}

static void nb_report_default(void)
{
	printf("[%3d sec] [%2d threads] %7d req/s %7d read/s %7d write/s  %.2f ms\n",
	       nb.tick,
	       nb.workers.count,
	       nb.stats.current->ps_req,
	       nb.stats.current->ps_read,
	       nb.stats.current->ps_write,
	       nb.stats.current->latency_req * 1000.0);
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
	"| rtt/ms   |    %.5f    |    %.5f    |    %.6f   |\n"
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
	       nb.stats.final.ps_req_max,
	       nb.stats.final.latency_req_min * 1000.0,
	       nb.stats.final.latency_req_avg * 1000.0,
	       nb.stats.final.latency_req_max * 1000.0);
}

struct nb_report_if nb_reports[] =
{
	{
		.name = "default",
		.init = NULL,
		.free = NULL,
		.report_start = nb_report_start,
		.report = nb_report_default,
		.report_final = nb_report_default_final
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
