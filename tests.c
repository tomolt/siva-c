#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SD_OPTION_PEDANTIC 1
#define SD_IMPLEMENT_HERE 1
#include "sd_cuts.h"
#include "table.h"

#define NUM_ITERATIONS 5000

static void test_table_insertion(struct siva_table * table,
	struct siva_key * keys, struct siva_entry * entries)
{
	sd_push("insertion");
	for (int i = 0; i < NUM_ITERATIONS; ++i) {
		sd_push("#%d", i);
		int rd = rand() % 0xFFFF;
		char buf[50];
		sprintf(buf, "%x", rd);
		int length = strlen(buf);
		char * name = calloc(1, length);
		strcpy(name, buf);
		struct siva_key key = { name, length, siva_table_hash_func(name, length) };
		struct siva_entry entry = { rd };
		siva_table_set(table, key, entry);
		keys[i] = key;
		entries[i] = entry;
		sd_pop();
	}
	sd_pop();
}

static void test_table_lookup(struct siva_table * table,
	struct siva_key * keys, struct siva_entry * entries)
{
	sd_push("lookup");
	for (int i = 0; i < NUM_ITERATIONS; ++i) {
		sd_push("#%d", i);
		struct siva_entry * entry = siva_table_get(table, keys[i]);
		sd_assert(entry != NULL);
		if (entry != NULL) {
			sd_assertiq(entry->value, entries[i].value);
		}
		sd_pop();
	}
	sd_pop();
}

static void test_table(void)
{
	sd_push("struct siva_table");
	struct siva_table table;
	siva_table_new(8, &table);
	struct siva_key keys[NUM_ITERATIONS];
	struct siva_entry entries[NUM_ITERATIONS];
	test_table_insertion(&table, keys, entries);
	test_table_lookup(&table, keys, entries);
	siva_table_free(&table);
	sd_pop();
}

int main()
{
	sd_init(stdout);
	sd_branch (
		test_table();
	)
	sd_summarize();
	return 0;
}

