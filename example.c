#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "siva.h"

int64_t unixfs_length(int fd)
{
	struct stat stats;
	if (fstat(fd, &stats) != 0)
		return -1;
	if (!S_ISREG(stats.st_mode))
		return -1;
	return stats.st_size;
}

int64_t unixfs_read(void * opaque, void * buffer, uint64_t size, uint64_t offset)
{
	int fd = (intptr_t) opaque;
	return pread(fd, buffer, size, offset);
}

#include <stdio.h>

int main()
{
	int fd = open("test.siva", O_RDONLY);
	if (fd < 0) {
		perror("open()");
		return -1;
	}
	int64_t length = unixfs_length(fd);
	if (length < 0) {
		fprintf(stderr, "unixfs_length()\n");
		return -1;
	}
	struct siva_io io = { (void *) (intptr_t) fd, length, unixfs_read };
	struct siva_archive * siva = siva_openarchive(io);
	if (siva == NULL) {
		fprintf(stderr, "siva_openarchive()\n");
		return -1;
	}
	for (uint64_t idx = 0; idx < siva->table.size; ++idx) {
		struct siva_key key = siva->table.keys[idx];
		struct siva_entry entry = siva->table.entries[idx];
		if (key.name == NULL) continue;
		if (entry.flags & 0x01) continue;
		printf("%s\n", key.name);
	}
	close(fd);
	return 0;
}

