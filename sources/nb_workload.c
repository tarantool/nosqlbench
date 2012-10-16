
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
	while(1) {
		if (workload->head == NULL)
			return NULL;
		if (workload->current) {
			if (workload->current->count == workload->requested)
				nb_workload_unlink(workload);
			workload->current = workload->current->next;
			if (workload->current == NULL)
				workload->current = workload->head;
		} else
			workload->current = workload->head;
		break;
	}
	return workload->current;
}
