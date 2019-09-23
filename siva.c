#include "siva.h"

#include <stdlib.h>
#include <stdio.h>

struct siva_io {
	void * opaque;
	int64_t (*read)(struct siva_io *, void *, uint64_t);
	int64_t (*write)(struct siva_io *, void const *, uint64_t);
	int64_t (*seek)(struct siva_io *, uint64_t);
	int64_t (*tell)(struct siva_io *);
	int64_t (*length)(struct siva_io *);
	int (*flush)(struct siva_io *);
};

struct siva;

struct siva * siva_openarchive(struct siva_io);
siva_open(struct siva *, char const *);
int64_t siva_read();
int64_t siva_write();
int64_t siva_seek();
int64_t siva_tell();
int64_t siva_length();
int siva_flush();

struct siva_entry {
	uint64_t fileOffset;
	uint64_t size;
	int64_t modTime;
	char const * name;
	uint32_t goMode;
	uint32_t lenName;
	unsigned int flags;
};

struct siva {
	struct siva_io io;
	struct siva_entry * entries;
	uint32_t numEntries;
	unsigned int ordered;
};

struct siva_footer {
	uint64_t indexSize;
	uint64_t blockSize;
	uint32_t numEntries;
	uint32_t crc32;
};

static inline uint32_t siva_getu32(uint8_t * restrict raw)
{
	uint32_t accum = 0;
	for (int i = 4; i > 0; --i)
		accum = (accum << 8) | *(raw++);
	return accum;
}

static inline uint64_t siva_getu64(uint8_t * restrict raw)
{
	uint64_t accum = 0;
	for (int i = 8; i > 0; --i)
		accum = (accum << 8) | *(raw++);
	return accum;
}

static int siva_readfooter(struct siva * siva, struct siva_footer * footer)
{
	uint8_t buffer[24];
	if (siva->io.seek(siva->io, end - 24) != )
		goto abort;
	if (siva->io.read(siva->io, buffer, 24) != 24)
		goto abort;
	footer->numEntries = siva_getu32(buffer + 0);
	uint64_t indexSize = siva_getu64(buffer + 4);
	uint64_t blockSize = siva_getu64(buffer + 12);
	footer->crc32 = siva_getu32(buffer + 20);
	if (indexSize < 24 || indexSize > blockSize)
		goto abort;
	if (blockSize < 24 || blockSize > end)
		goto abort;
	footer->indexOffset = end - indexSize;
	footer->blockOffset = end - blockSize;
	return 1;
abort:
	return 0;
}

static int siva_readentries(struct siva * siva)
{
	uint64_t entriesSize = indexSize - 24;
	char * buffer = calloc(1, entriesSize);
	if (io->seek(io, entriesOffset) != )
		goto abort;
	if (io->read(io, buffer, entriesSize) != entriesSize)
		goto abort;
	if (memcmp(buffer, "IBA1", 4) != 0)
		goto abort;
	if (siva_crc32(buffer, entriesSize) != crc32)
		goto abort;
	for (int i = 0; i < numEntries; ++i) {
		struct siva_entry * entry = ;
		entry->lenName = siva_getu32(cursor);
	}
	free(buffer);
	return 1;
abort:
	free(buffer);
	return 0;
}

struct siva * siva_openarchive(struct siva_io io)
{
	struct siva * siva = calloc(1, sizeof(struct siva));
	siva->io = io;
	int64_t end = siva->io.length(siva->io);
	do {
		if (end < 24)
			goto abort;
		if (!siva_readfooter(siva))
			goto abort;
		if (!siva_readentries(siva))
			goto abort;
		end = ;
	} while (end > 0);
	return siva;
abort:
	free(siva);
	return NULL;
}

