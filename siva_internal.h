
uint32_t siva_table_hash_func(char const *, uint32_t);
void siva_table_new(uint64_t, struct siva_table *);
void siva_table_free(struct siva_table *);
void siva_table_set(struct siva_table *, struct siva_key, struct siva_entry);
struct siva_entry * siva_table_get(struct siva_table *, struct siva_key);
uint32_t siva_crc32(uint8_t const * restrict message, uint64_t count);

