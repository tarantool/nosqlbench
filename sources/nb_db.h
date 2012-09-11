#ifndef NB_DB_H_INCLUDED
#define NB_DB_H_INCLUDED

struct nb_db;
struct nb_key;

typedef int (*nb_db_reqf_t)(struct nb_db *db, struct nb_key *key);

struct nb_db_if {
	const char *name;
	int (*init)(struct nb_db *db, size_t value_size);
	void (*free)(struct nb_db *db);
	int (*connect)(struct nb_db *db, char *host, int port);
	void (*close)(struct nb_db *db);
	int (*recv)(struct nb_db *db, int count, int *missed);
	nb_db_reqf_t insert;
	nb_db_reqf_t replace;
	nb_db_reqf_t update;
	nb_db_reqf_t del;
	nb_db_reqf_t select;
};

struct nb_db {
	struct nb_db_if *dif;
	void *priv;
};

extern struct nb_db_if *nb_dbs[];

struct nb_db_if *nb_db_match(const char *name);

#endif
