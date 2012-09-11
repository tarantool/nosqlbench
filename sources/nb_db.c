
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "nb_alloc.h"
#include "nb_key.h"
#include "nb_db.h"
#include "nb_db_tarantool.h"

struct nb_db_if *nb_dbs[] =
{
	&nb_db_tarantool,
	NULL,
};

struct nb_db_if *nb_db_match(const char *name)
{
	int i = 0;
	while (nb_dbs[i]) {
		if (strcmp(nb_dbs[i]->name, name) == 0)
			return nb_dbs[i];
		i++;
	}
	return NULL;
}
