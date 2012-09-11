#ifndef NB_ALLOC_H_INCLUDED
#define NB_ALLOC_H_INCLUDED

static inline void nb_oom(void *ptr) {
	if (ptr == NULL) {
		printf("memory allocation failed\n");
		abort();
	}
}

static inline void *nb_malloc(size_t size) {
	void *ptr = malloc(size);
	nb_oom(ptr);
	return ptr;
}

static inline void *nb_realloc(char *ptr, size_t size) {
	void *nptr = realloc(ptr, size);
	nb_oom(nptr);
	return nptr;
}

#endif
