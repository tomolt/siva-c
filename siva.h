#ifndef SIVA_H
#define SIVA_H

#include <stdint.h>

struct siva_io {
	void * opaque;
	uint64_t length;
	int64_t (*read)(void *, void *, uint64_t, uint64_t);
};

struct siva_key {
	char * name;
	uint32_t length;
	uint32_t hash;
};

struct siva_entry {
	uint64_t fileOffset;
	uint64_t size;
	int64_t modTime;
	uint32_t goMode;
	uint32_t crc32;
	uint8_t flags;
};

struct siva_table {
	uint64_t size;
	uint64_t count;
	struct siva_key * keys;
	struct siva_entry * entries;
};

struct siva_archive {
	struct siva_io io;
	struct siva_table table;
};

struct siva_archive * siva_openarchive(struct siva_io);
void siva_freearchive(struct siva_archive *);

#endif

