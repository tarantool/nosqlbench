#ifndef NB_WORKLOAD_H_INCLUDED
#define NB_WORKLOAD_H_INCLUDED

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
