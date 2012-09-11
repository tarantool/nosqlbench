#ifndef NB_KEY_H_INCLUDED
#define NB_KEY_H_INCLUDED

typedef unsigned int (*nb_key_randomf_t)(unsigned int max);

struct nb_key_distribution_if {
	const char *name;
	void (*init)(int iterations);
	nb_key_randomf_t random;
};

struct nb_key {
	char *data;
	size_t size;
	struct nb_key_distribution_if *distif;
};

struct nb_key_if {
	const char *name;
	void (*init)(struct nb_key *key, struct nb_key_distribution_if *distif);
	void (*free)(struct nb_key *key);
	void (*generate)(struct nb_key *key, uint32_t max);
	void (*generate_by_id)(struct nb_key *key, uint32_t id);
};

extern struct nb_key_if nb_keys[];
extern struct nb_key_distribution_if nb_key_dists[];

struct nb_key_if *nb_key_match(const char *name);

struct nb_key_distribution_if*
nb_key_distribution_match(const char *name);

#endif
