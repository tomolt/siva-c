#include <stdlib.h>
#include <string.h>

#include "siva.h"
#include "siva_internal.h"

static inline uint32_t siva_getu32(uint8_t ** cursor)
{
	uint8_t * restrict ptr = *cursor;
	uint32_t accum = 0;
	for (int i = 4; i > 0; --i)
		accum = (accum << 8) | *(ptr++);
	*cursor = ptr;
	return accum;
}

static inline uint64_t siva_getu64(uint8_t ** cursor)
{
	uint8_t * restrict ptr = *cursor;
	uint64_t accum = 0;
	for (int i = 8; i > 0; --i)
		accum = (accum << 8) | *(ptr++);
	*cursor = ptr;
	return accum;
}

static int siva_readentry(
	uint8_t ** cursor,
	uint64_t blockOffset,
	struct siva_table * table)
{
	struct siva_key key;
	struct siva_entry entry;
	/* read and store all the primitive data fields */
	key.length = siva_getu32(cursor);
	key.name = calloc(1, key.length + 1);
	memcpy(key.name, *cursor, key.length);
	*cursor += key.length;
	key.hash = siva_table_hash_func(key.name, key.length);
	entry.goMode = siva_getu32(cursor);
	entry.modTime = (int64_t) siva_getu64(cursor);
	entry.fileOffset = blockOffset + siva_getu64(cursor);
	entry.size = siva_getu64(cursor);
	entry.crc32 = siva_getu32(cursor);
	entry.flags = siva_getu32(cursor);
	/* perform some simple sanity checks */
	if (entry.flags != 0x0 && entry.flags != 0x1)
		goto abort;
	/* success */
	siva_table_set(table, key, entry);
	return 1;
	/* failure */
abort:
	return 0;
}

struct siva_footer
{
	uint64_t indexOffset;
	uint64_t blockOffset;
	uint64_t indexSize;
	uint32_t numEntries;
	uint32_t crc32;
};

static int siva_readindex(
	struct siva_io io,
	struct siva_footer footer,
	struct siva_table * table)
{
	/* copy relevant memory into a buffer */
	uint8_t * buffer = calloc(1, footer.indexSize);
	if (buffer == NULL)
		return 0;
	if (io.read(io.opaque, buffer, footer.indexSize,
		footer.indexOffset) != (int64_t) footer.indexSize)
		goto abort;
	/* verify integrity of buffer contents */
	if (memcmp(buffer, "IBA\01", 4) != 0)
		goto abort;
	if (siva_crc32(buffer, footer.indexSize) != footer.crc32)
		goto abort;
	/* read & append entries one by one */
	uint8_t * ptr = buffer + 4, ** cursor = &ptr;
	for (uint32_t idx = 0; idx < footer.numEntries; ++idx) {
		if (!siva_readentry(cursor, footer.blockOffset, table))
			goto abort;
	}
	/* success */
	free(buffer);
	return 1;
	/* failure */
abort:
	free(buffer);
	return 0;
}

static int siva_readfooter(
	struct siva_io io,
	uint64_t * end,
	struct siva_footer * footer)
{
	unsigned int const FOOTER_SIZE = 24;
	/* copy relevant memory into a buffer */
	uint8_t buffer[FOOTER_SIZE];
	if (*end < FOOTER_SIZE)
		goto abort;
	if (io.read(io.opaque, buffer, FOOTER_SIZE, *end - FOOTER_SIZE) != FOOTER_SIZE)
		goto abort;
	/* read and store all the primitive data fields */
	uint8_t * ptr = buffer, ** cursor = &ptr;
	footer->numEntries = siva_getu32(cursor);
	footer->indexSize = siva_getu64(cursor);
	uint64_t blockSize = siva_getu64(cursor);
	footer->crc32 = siva_getu32(cursor);
	footer->indexOffset = *end - FOOTER_SIZE - footer->indexSize;
	footer->blockOffset = *end - blockSize;
	/* perform some simple sanity checks */
	if (footer->indexSize < FOOTER_SIZE || footer->indexSize > blockSize)
		goto abort;
	if (blockSize < FOOTER_SIZE || blockSize > *end)
		goto abort;
	/* move to the end of the preceding block */
	*end -= blockSize;
	/* success */
	return 1;
	/* failure */
abort:
	return 0;
}

struct siva_archive * siva_openarchive(struct siva_io io)
{
	/* allocate space for the archive */
	struct siva_archive * siva = calloc(1, sizeof(struct siva_archive));
	if (siva == NULL)
		return NULL;
	siva->io = io;
	siva_table_new(16, &siva->table);
	/* iterate through all blocks and concatenate contents, most recent coming first */
	struct siva_footer footers[100]; /* TODO dynamic (re-) allocation */
	uint64_t count = 0;
	uint64_t end = io.length;
	do {
		if (!siva_readfooter(io, &end, &footers[count++]))
			goto abort;
	} while (end > 0);
	for (int64_t idx = count - 1; idx >= 0; --idx) {
		if (!siva_readindex(io, footers[idx], &siva->table))
			goto abort;
	}
	/* success */
	return siva;
	/* failure */
abort:
	siva_freearchive(siva);
	return NULL;
}

void siva_freearchive(struct siva_archive * archive)
{
	(void) archive;
	/* TODO */
}

