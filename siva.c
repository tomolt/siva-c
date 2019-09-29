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
	uint64_t baseOffset,
	struct siva_entry ** outEntry)
{
	/* allocate entry with enough space for its name */
	uint32_t lenName = siva_getu32(cursor);
	struct siva_entry * entry = calloc(1, sizeof(struct siva_entry));
	if (entry == NULL)
		return 0;
	/* read and store all the primitive data fields */
	entry->lenName = lenName;
	memcpy(entry->name, *cursor, lenName);
	*cursor += lenName;
	entry->goMode = siva_getu32(cursor);
	entry->modTime = (int64_t) siva_getu64(cursor);
	entry->fileOffset = baseOffset + siva_getu64(cursor);
	entry->size = siva_getu64(cursor);
	entry->crc32 = siva_getu32(cursor);
	entry->flags = siva_getu32(cursor);
	/* perform some simple sanity checks */
	if (entry->flags != 0x0 && entry->flags != 0x1)
		goto abort;
	/* success */
	*outEntry = entry;
	return 1;
	/* failure */
abort:
	free(entry);
	return 0;
}

static int siva_readentries(
	struct siva_io io,
	uint64_t entriesOffset,
	uint64_t entriesSize,
	uint32_t numEntries,
	uint64_t baseOffset,
	uint32_t crc32,
	struct siva_archive ** siva)
{
	/* copy relevant memory into a buffer */
	uint8_t * buffer = calloc(1, entriesSize);
	if (buffer == NULL)
		return 0;
	if (io.read(io.opaque, buffer, entriesSize, entriesOffset) != (int64_t) entriesSize)
		goto abort;
	/* verify integrity of buffer contents */
	if (memcmp(buffer, "IBA\01", 4) != 0)
		goto abort;
	if (siva_crc32(buffer, entriesSize) != crc32)
		goto abort;
	/* make space for new entries */
	uint32_t baseIndex = (*siva)->numEntries;
	(*siva)->numEntries += numEntries;
	*siva = realloc(*siva, sizeof(struct siva_archive) + (*siva)->numEntries * sizeof(struct siva_entry));
	if (*siva == NULL)
		goto abort;
	/* read & append entries one by one */
	uint8_t * ptr = buffer + 4, ** cursor = &ptr;
	for (uint32_t idx = 0; idx < numEntries; ++idx) {
		struct siva_entry * entry;
		if (!siva_readentry(cursor, baseOffset, &entry))
			goto abort;
		(*siva)->entries[baseIndex + idx] = entry;
	}
	/* success */
	free(buffer);
	return 1;
	/* failure */
abort:
	free(buffer);
	return 0;
}

static int siva_readindex(
	struct siva_io io,
	uint64_t * end,
	struct siva_archive ** siva)
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
	uint32_t numEntries = siva_getu32(cursor);
	uint64_t indexSize = siva_getu64(cursor);
	uint64_t blockSize = siva_getu64(cursor);
	uint32_t crc32 = siva_getu32(cursor);
	/* perform some simple sanity checks */
	if (indexSize < FOOTER_SIZE || indexSize > blockSize)
		goto abort;
	if (blockSize < FOOTER_SIZE || blockSize > *end)
		goto abort;
	/* read the list of entries */
	if (!siva_readentries(io, *end - FOOTER_SIZE - indexSize,
		indexSize, numEntries, *end - blockSize, crc32, siva))
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
	/* iterate through all blocks and concatenate contents, most recent coming first */
	uint64_t end = io.length;
	do {
		if (!siva_readindex(io, &end, &siva))
			goto abort;
	} while (end > 0);
	/* success */
	return siva;
	/* failure */
abort:
	free(siva);
	return NULL;
}

