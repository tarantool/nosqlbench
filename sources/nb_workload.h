#ifndef NB_WORKLOAD_H_INCLUDED
#define NB_WORKLOAD_H_INCLUDED

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

enum nb_request_type {
	NB_REPLACE,
	NB_UPDATE,
	NB_DELETE,
	NB_SELECT,
	NB_REQUEST_MAX,
	/* insert doesn't present in tests */
	NB_INSERT
};

struct nb_request {
	enum nb_request_type type;
	int count;
	int requested;
	int percent;
	nb_db_reqf_t _do; 
	struct nb_request *next, *prev;
};

struct nb_workload {
	int count;
	int count_write;
	int count_read;
	int requested, processed;
	struct nb_request reqs[NB_REQUEST_MAX];
	struct nb_request *head;
	struct nb_request *current;
};

void nb_workload_init(struct nb_workload *workload, int count);
void nb_workload_init_from(struct nb_workload *dest, struct nb_workload *src);
void nb_workload_link(struct nb_workload *workload);
void nb_workload_reset(struct nb_workload *workload);

void nb_workload_add(struct nb_workload *workload, enum nb_request_type type,
		     nb_db_reqf_t req,
		     int percent);

struct nb_request *nb_workload_fetch(struct nb_workload *workload);

#endif
