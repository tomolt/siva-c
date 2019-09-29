#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "siva.h"
#include "siva_internal.h"

static int const siva_table_load_factor = 80;

uint32_t siva_table_hash_func(char const * name, uint32_t length)
{
	uint32_t hash = 33;
	for (uint32_t i = 0; i < length; ++i) {
		hash += name[i];
		hash += hash << 10;
		hash ^= hash >> 6;
	}
	hash += hash << 3;
	hash ^= hash >> 11;
	hash += hash << 15;
	return hash;
}

void siva_table_new(uint64_t size, struct siva_table * table)
{
	table->size = size;
	table->count = 0;
	table->keys = calloc(size, sizeof(*table->keys));
	table->entries = calloc(size, sizeof(*table->entries));
	for (uint64_t idx = 0; idx < size; ++idx) {
		table->keys[idx].hash = idx;
	}
}

void siva_table_free(struct siva_table * table)
{
	free(table->keys);
	free(table->entries);
}

static int siva_table_probe(struct siva_table * table, struct siva_key key, uint64_t * ret)
{
	uint64_t distance = 0;
	for (;;) {
		uint64_t idx = (key.hash + distance) % table->size;
		struct siva_key prKey = table->keys[idx];
		int doesMatch = key.hash == prKey.hash &&
			key.length == prKey.length &&
			memcmp(key.name, prKey.name, key.length) == 0;
		if (doesMatch) {
			*ret = idx;
			return 1;
		}
		int64_t prDistance = idx - (prKey.hash % table->size);
		if (prDistance < 0) {
			prDistance += table->size;
		}
		if (distance > (uint64_t) prDistance) {
			*ret = idx;
			return 0;
		}
		++distance;
	}
}

static void siva_table_insert(struct siva_table * table,
	struct siva_key key, struct siva_entry entry)
{
	++table->count;
	do {
		uint64_t idx;
		if (siva_table_probe(table, key, &idx)) {
			table->entries[idx] = entry;
			--table->count;
			key.name = NULL;
		} else {
			struct siva_key tmpKey = key;
			key = table->keys[idx];
			table->keys[idx] = tmpKey;
			struct siva_entry tmpEntry = entry;
			entry = table->entries[idx];
			table->entries[idx] = tmpEntry;
		}
	} while (key.name != NULL);
}

void siva_table_set(struct siva_table * table,
	struct siva_key key, struct siva_entry entry)
{
	if ((int) (100 * (table->count + 1) / table->size) > siva_table_load_factor) {
		struct siva_table repl;
		siva_table_new(table->size * 2, &repl);
		for (uint64_t idx = 0; idx < table->size; ++idx) {
			if (table->keys[idx].name != NULL) {
				siva_table_insert(&repl, table->keys[idx], table->entries[idx]);
			}
		}
		siva_table_free(table);
		memcpy(table, &repl, sizeof(repl));
	}
	siva_table_insert(table, key, entry);
}

struct siva_entry * siva_table_get(struct siva_table * table, struct siva_key key)
{
	uint64_t idx;
	if (siva_table_probe(table, key, &idx)) {
		return &table->entries[idx];
	} else {
		return NULL;
	}
}

