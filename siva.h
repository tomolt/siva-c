#ifndef SIVA_H
#define SIVA_H

#include <stdint.h>

struct siva_io {
	void * opaque;
	uint64_t length;
	int64_t (*read)(void *, void *, uint64_t, uint64_t);
};

struct siva_entry {
	uint64_t fileOffset;
	uint64_t size;
	int64_t modTime;
	uint32_t lenName;
	uint32_t goMode;
	uint32_t crc32;
	uint8_t flags;
	char name[];
};

struct siva_archive {
	struct siva_io io;
	uint32_t numEntries;
	struct siva_entry * entries[];
};

struct siva_archive * siva_openarchive(struct siva_io io);

#endif

