#ifndef SIVA_TABLE_H
#define SIVA_TABLE_H

struct siva_key {
	char const * name;
	uint32_t length;
	uint32_t hash;
};

struct siva_table {
	uint64_t size;
	uint64_t count;
	struct siva_key * keys;
	struct siva_entry * entries;
};

uint32_t siva_table_hash_func(char const *, uint32_t);
void siva_table_new(uint64_t, struct siva_table *);
void siva_table_free(struct siva_table *);
void siva_table_set(struct siva_table *, struct siva_key, struct siva_entry);
struct siva_entry * siva_table_get(struct siva_table *, struct siva_key);

#endif

