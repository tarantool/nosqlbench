
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>

#include "nb_alloc.h"
#include "nb_key.h"
#include "nb_db.h"
#include "nb_workload.h"

void nb_workload_link(struct nb_workload *workload) {
	int i = 0;
	struct nb_request *head = NULL;
	struct nb_request *tail = NULL;
	while (i < NB_REQUEST_MAX) {
		if (workload->reqs[i].count == 0) {
			i++;
			continue;
		}
		if (head == NULL)
			head = &workload->reqs[i];
		else
			tail->next = &workload->reqs[i];
		tail = &workload->reqs[i];
		i++;
	}
	workload->head = head;
}

static void nb_workload_unlink(struct nb_workload *workload) {
	struct nb_request *c = workload->current;
	if (c->prev)
		c->prev->next = c->next;
	if (c->next)
		c->next->prev = c->prev;
	if (c == workload->head)
		workload->head = c->next;
}

void nb_workload_init(struct nb_workload *workload, int count)
{
	workload->count = count;
	workload->count_write = 0;
	workload->count_read = 0;
	workload->requested = 0;
	workload->processed = 0;
	workload->head = NULL;
	workload->current = NULL;
	memset(workload->reqs, 0, sizeof(workload->reqs));
}

void nb_workload_init_from(struct nb_workload *dest, struct nb_workload *src)
{
	nb_workload_init(dest, src->count);
	int i = 0;
	while (i < NB_REQUEST_MAX) {
		nb_workload_add(dest, src->reqs[i].type,
				src->reqs[i]._do,
				src->reqs[i].percent);
		i++;
	}
}


void nb_workload_add(struct nb_workload *workload, enum nb_request_type type,
		     nb_db_reqf_t req, int percent)
{
	struct nb_request *r = &workload->reqs[type];
	r->type = type;
	r->percent = percent;
	r->requested = 0;
	r->count = (int)(workload->count * r->percent / 100.0);
	r->_do = req;
}

void nb_workload_reset(struct nb_workload *workload)
{
	workload->processed = 0;
	workload->requested = 0;
	int i = 0;
	while (i < NB_REQUEST_MAX) {
		workload->reqs[i].requested = 0;
		i++;
	}
	workload->head = NULL;
	workload->current = NULL;
	nb_workload_link(workload);
}

struct nb_request *nb_workload_fetch(struct nb_workload *workload)
{
	do {
		if (workload->head == NULL)
			return NULL;
		if (workload->current) {
			if (workload->current->count == workload->requested) {
				nb_workload_unlink(workload);
				continue;
			}
			workload->current = workload->current->next;
			if (workload->current == NULL)
				workload->current = workload->head;
		} else
			workload->current = workload->head;
		break;
	} while (0);

	return workload->current;
}
