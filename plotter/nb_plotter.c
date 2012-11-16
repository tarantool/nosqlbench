
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
#include <stdio.h>
#include <string.h>

struct nb_plot_metric {
	int workers;
	int time;
	float req;
	float read;
	float write; 
	float latency; 
	int avg_count;
	struct nb_plot_metric *next;
};

struct nb_plot {
	struct nb_plot_metric *head;
	struct nb_plot_metric *tail;
	int count;
};

void nb_plot_init(struct nb_plot *p) {
	p->head = NULL;
	p->tail = NULL;
	p->count++;
}

void nb_plot_free(struct nb_plot *p) {
	struct nb_plot_metric *nn, *n = p->head;
	if (n) {
		nn = n->next;
		free(n);
		n = nn;
	}
}

static struct nb_plot_metric*
nb_plot_append(struct nb_plot *p) {
	struct nb_plot_metric *m = malloc(sizeof(struct nb_plot_metric));
	if (m == NULL)
		return NULL;
	memset(m, 0, sizeof(m));
	if (p->head == NULL)
		p->head = m;
	else
		p->tail->next = m;
	p->tail = m;
	p->count++;
	return m;
}

static struct nb_plot_metric*
nb_plot_match(struct nb_plot *p, int workers) {
	struct nb_plot_metric *m = p->head;
	while (m) {
		if (m->workers == workers)
			return m;
		m = m->next;
	}
	return NULL;
}

static void nb_plot_add(struct nb_plot *p, struct nb_plot_metric *m) {
	struct nb_plot_metric *n = nb_plot_match(p, m->workers);
	if (n) {
		n->time += m->time;
		n->req += m->req;
		n->read += m->read;
		n->write += m->write;
		n->latency += m->latency;
		n->avg_count++;
		return;
	}
	n = nb_plot_append(p);
	if (n == NULL)
		return;
	memcpy(n, m, sizeof(struct nb_plot_metric));
}

static void nb_plot_complete(struct nb_plot *p) {
	struct nb_plot_metric *n = p->head;
	while (n) {
		if (n->avg_count > 1) {
			n->time /= n->avg_count;
			n->req /= n->avg_count;
			n->read /= n->avg_count;
			n->write /= n->avg_count;
			n->latency /= n->avg_count;
		}
		n = n->next;
	}
}

static void nb_plot_generate(struct nb_plot *p, char *path) {
	FILE *f = fopen(path, "w");
	struct nb_plot_metric *n = p->head;
	while (n) {
		fprintf(f, "%i, %i, %.2f, %.2f, %.2f, %f\n",
			n->workers,
			n->time,
			n->req,
			n->read,
			n->write,
			n->latency);
		n = n->next;
	}
	fclose(f);
}

static int nb_plot_min_workers(struct nb_plot *p) {
	if (p->head == NULL)
		return -1;
	int min = p->head->workers;
	struct nb_plot_metric *n = p->head->next;
	while (n) {
		if (n->workers < min)
			min = n->workers;
		n = n->next;
	}
	return min;
}

static int nb_plot_max_workers(struct nb_plot *p) {
	if (p->head == NULL)
		return -1;
	int max = 0;
	struct nb_plot_metric *n = p->head;
	while (n) {
		if (n->workers > max)
			max = n->workers;
		n = n->next;
	}
	return max;
}

static char *nb_plot_list_workers(struct nb_plot *p) {
	static char list[8096];
	int pos = 0;
	int first = 1;
	struct nb_plot_metric *n = p->head;
	while (n) {
		if (first) {
			pos += snprintf(list + pos, sizeof(list) - pos, "%d", n->workers);
			first = 0;
		} else {
			pos += snprintf(list + pos, sizeof(list) - pos, ", %d", n->workers);
		}
		n = n->next;
	}
	return list;
}

static void
nb_plot_create_rps_vs_threads(struct nb_plot *p, char *csv_file, char *plot_file, char *title) {
	char *head =
		"set terminal png nocrop enhanced size 1280,768 fon Arial 7\n"
		"set output '%s.png'\n"
		"set datafile separator ','\n"
		"set style fill pattern 2\n"
		"set ytics border in scale 1,0.5 mirror norotate offset character 0, 0, 0\n"
		"set xtics (%s)\n"
		"set title \"%s\"\n"
		"set xlabel \"Clients\"\n"
		"set ylabel \"Rps\"\n"
		"set xrange [%d : %d] noreverse nowriteback\n\n";

	char *plot = 
		 "plot '%s' using 1:3 with filledcu t \"total\", \\\n"
		 "'%s' using 1:3:3 with labels notitle, \\\n"
		 "'%s' using 1:4 with filledcu t \"read\", \\\n"
		 "'%s' using 1:4:4 with labels notitle, \\\n"
		 "'%s' using 1:5 with filledcu t \"write\", \\\n"
		 "'%s' using 1:5:5 with labels notitle\n";

	FILE *f = fopen(plot_file, "w");
	if (f == NULL)
		return;

	int min = nb_plot_min_workers(p);
	int max = nb_plot_max_workers(p);

	fprintf(f, head, plot_file, nb_plot_list_workers(p), title,
		min, max);

	fprintf(f, plot,
		csv_file, csv_file, csv_file,
		csv_file, csv_file, csv_file);

	fclose(f);
}

int main(int argc, char * argv[])
{
	if (argc != 5) {
		printf("NoSQL Benchmarking Plotter.\n");
		printf("usage: %s <benchmark.csv> <output.csv> <output.plot> <plot title>\n",
		       argv[0]);
		return 1;
	}

	FILE *f = fopen(argv[1], "r");
	if (f == NULL) {
		return 1;
	}

	struct nb_plot p;
	nb_plot_init(&p);

	while (1) {
		struct nb_plot_metric m = { .next = NULL };

		int rc = fscanf(f, "%i, %i, %f, %f, %f, %f",
				&m.workers,
				&m.time,
				&m.req,
				&m.read,
				&m.write,
				&m.latency);
		if (rc != 6)
			break;
		nb_plot_add(&p, &m);
	}

	nb_plot_complete(&p);
	nb_plot_generate(&p, argv[2]);
	nb_plot_create_rps_vs_threads(&p, argv[2], argv[3], argv[4]);
	nb_plot_free(&p);

	fclose(f);
	return 0;
}
