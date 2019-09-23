#include "siva.h"

#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>

#define SIVA_ALRIGHT 0
#define SIVA_ARCHIVE_CORRUPTED -1
#define SIVA_FILE_INACCESSIBLE -2
#define SIVA_OUT_OF_MEMORY -3

#define EOK SIVA_ALRIGHT
#define EAC SIVA_ARCHIVE_CORRUPTED
#define EFI SIVA_FILE_INACCESSIBLE
#define EOM SIVA_OUT_OF_MEMORY

struct Entry {
	uint64_t blockOffset;
	uint64_t size;
	int64_t modTime;
	const char * name;
	uint32_t goMode;
	uint32_t lenName;
	unsigned int flags;
};

struct Block {
	uint64_t fileOffset;
	struct Entry * entries;
	uint32_t numEntries;
};

struct SivaFile {
	struct Block * blocks;
	unsigned int numBlocks;
};

static inline void enforce(int cond, int nature, jmp_buf * err)
{
	if (!cond) {
		longjmp(*err, nature);
	}
}

static uint32_t getu32(FILE * file, jmp_buf * err)
{
	uint32_t accum = 0;
	for (int i = 0; i < 4; ++i) {
		int byte = fgetc(file);
		enforce(byte != EOF, EFI, err);
		accum = accum << 8 | byte;
	}
	return accum;
}

static uint64_t getu64(FILE * file, jmp_buf * err)
{
	uint64_t accum = 0;
	for (int i = 0; i < 8; ++i) {
		int byte = fgetc(file);
		enforce(byte != EOF, EFI, err);
		accum = accum << 8 | byte;
	}
	return accum;
}

static uint64_t getFileSize(FILE * file, jmp_buf * err)
{
	struct stat stats;
	int fd = fileno(file);
	enforce(fstat(fd, &stats) == 0, EFI, err);
	enforce(S_ISREG(stats.st_mode), EFI, err);
	return stats.st_size;
}

int sivaReadFile(FILE * file)
{
	jmp_buf jmp, * err = &jmp;
	int ret = setjmp(jmp);
	if (ret == 0) {
		uint64_t leftSize = getFileSize(file, err);
		do {
			/* footer */
			enforce(leftSize >= 24, EAC, err);
			uint64_t indexSize, blockSize;
			fseeko(file, leftSize - 24, SEEK_SET);
			uint32_t numEntries = getu32(file, err);
			struct Block * block = allocBlock(numEntries);
			uint64_t indexSize = getu64(file, err);
			uint64_t blockSize = getu64(file, err);
			uint32_t crc32 = getu32(file, err);
			(void) crc32; // TODO integrity check
			enforce(indexSize >= 24 && indexSize <= blockSize, EAC, err);
			enforce(blockSize >= 24 && blockSize <= leftSize, EAC, err);
			/* index */
			fseeko(file, leftSize - indexSize, SEEK_SET);
			char magic[4];
			enforce(fread(magic, 4, 1, file) == 1, EFI, err);
			enforce(memcmp(magic, "IBA1", 4) == 0, EAC, err);
			for (unsigned int e = 0; e < numEntries; ++e) {
				struct Entry * entry = block->entries + e;
				entry->lenName = getu32(file, err);
				entry->name = calloc(entry->lenName + 1, 1);
				enforce(fread(entry->name, entry->lenName, 1, file) == 1, EFI, err);
				entry->goMode = getu32(file, err);
				entry->modTime = getu64(file, err);
				entry->blockOffset = getu64(file, err);
				entry->size = getu64(file, err);
				entry->crc32 = getu32(file, err);
				entry->flags = getu32(file, err);
			}
			leftSize -= blockSize;
		} while (leftSize != 0);
		return EOK;
	} else {
		sivaFree(siva);
		return ret;
	}
}

