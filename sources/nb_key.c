
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
#include <time.h>

#include "nb_alloc.h"
#include "nb_key.h"

static void
nb_key_string_init(struct nb_key *key, struct nb_key_distribution_if *distif)
{
	/* 'K' + UINT_MAX(4294967295) + '\0' */
	key->size = 12;
	key->data = nb_malloc(key->size);
	key->distif = distif;
	snprintf(key->data, 12, "K%10d", 0);
}

static void
nb_key_string_free(struct nb_key *key)
{
	if (key->data)
		free(key->data);
}

static void
nb_key_string_generate(struct nb_key *key, unsigned int max)
{
	unsigned int kv = key->distif->random(max);
	snprintf(key->data, key->size, "K%10d", kv);
}

static void
nb_key_string_generate_id(struct nb_key *key, unsigned int id)
{
	snprintf(key->data, key->size, "K%10d", id);
}

static void
nb_key_u32_init(struct nb_key *key, struct nb_key_distribution_if *distif)
{
	key->size = sizeof(uint32_t);
	key->data = nb_malloc(key->size);
	key->distif = distif;
	*((uint32_t*)key->data) = 0;
}

static void
nb_key_u32_free(struct nb_key *key)
{
	if (key->data)
		free(key->data);
}

static void
nb_key_u32_generate(struct nb_key *key, unsigned int max)
{
	*((uint32_t*)key->data) =  key->distif->random(max);
}

static void
nb_key_u32_generate_id(struct nb_key *key, unsigned int id)
{
	*((uint32_t*)key->data) = id;
}

static void
nb_key_u64_init(struct nb_key *key, struct nb_key_distribution_if *distif)
{
	key->size = sizeof(uint64_t);
	key->data = nb_malloc(key->size);
	key->distif = distif;
	*((uint64_t*)key->data) = 0;
}

static void
nb_key_u64_free(struct nb_key *key)
{
	if (key->data)
		free(key->data);
}

static void
nb_key_u64_generate(struct nb_key *key, unsigned int max)
{
	*((uint64_t*)key->data) =  key->distif->random(max);
}

static void
nb_key_u64_generate_id(struct nb_key *key, unsigned int id)
{
	*((uint64_t*)key->data) = id;
}

struct nb_key_if nb_keys[] =
{
	{
		.name = "string",
		.init = nb_key_string_init,
		.free = nb_key_string_free,
		.generate = nb_key_string_generate,
		.generate_by_id = nb_key_string_generate_id
	},
	{
		.name = "u32",
		.init = nb_key_u32_init,
		.free = nb_key_u32_free,
		.generate = nb_key_u32_generate,
		.generate_by_id = nb_key_u32_generate_id
	},
	{
		.name = "u64",
		.init = nb_key_u64_init,
		.free = nb_key_u64_free,
		.generate = nb_key_u64_generate,
		.generate_by_id = nb_key_u64_generate_id
	},
	{
		.name = NULL
	}
};

struct nb_key_if *nb_key_match(const char *name)
{
	int i = 0;
	while (nb_keys[i].name) {
		if (strcmp(nb_keys[i].name, name) == 0)
			return &nb_keys[i];
		i++;
	}
	return NULL;
}

static void nb_dist_uniform_init(int iterations)
{
	(void)iterations;
	time_t tm = time(NULL);
	srandom((unsigned int)tm);
}

static unsigned int nb_dist_uniform_random(unsigned int max)
{
	return random() % max;
}

static int gaussian_iter = 0;

static void nb_dist_gaussian_init(int iterations)
{
	time_t tm = time(NULL);
	srandom((unsigned int)tm);
	if (iterations <= 0)
		iterations = 1;
	gaussian_iter = iterations;
}

static unsigned int nb_dist_gaussian_random(unsigned int max)
{
	int sum = 0;
	int i = 0;
	while (i < gaussian_iter) {
		sum += random() % max;
		i++;
	}
	return sum / gaussian_iter;
}

struct nb_key_distribution_if nb_key_dists[] =
{
	{
		.name = "uniform",
		.init = nb_dist_uniform_init,
		.random = nb_dist_uniform_random
	},
	{
		.name = "gaussian",
		.init = nb_dist_gaussian_init,
		.random = nb_dist_gaussian_random
	},
	{
		.name = NULL
	}
};

struct nb_key_distribution_if*
nb_key_distribution_match(const char *name)
{
	int i = 0;
	while (nb_key_dists[i].name) {
		if (strcmp(nb_key_dists[i].name, name) == 0)
			return &nb_key_dists[i];
		i++;
	}
	return NULL;
}
