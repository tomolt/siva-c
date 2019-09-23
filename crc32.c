#include <stdint.h>

#define POLYNOMIAL 0x04C11DB7

static uint32_t LUT[256];

static uint32_t reflect(uint32_t in, int count)
{
	uint32_t out = 0;
	for (int i = 0; i < count; ++i) {
		int j = count - 1 - i;
		out |= ((in >> i) & 1) << j;
	}
	return out;
}

void initCRC32(void)
{
	for (unsigned int byte = 0; byte < 256; ++byte) {
		uint32_t remainder = byte << 24;
		for (int i = 8; i > 0; --i) {
			int topbit = remainder & (1 << 31);
			remainder += remainder;
			if (topbit) {
				remainder ^= 0x04C11DB7;
			}
		}
		LUT[byte] = remainder;
	}
}

uint32_t sivaCRC32(uint8_t message[], uint64_t count)
{
	uint32_t remainder = 0xFFFFFFFF;	
	for (int i = 0; i < count; ++i) {
		uint32_t topbyte = reflect(message[i], 8) ^ (remainder >> 24);
		remainder = LUT[topbyte] ^ (remainder << 8);
	}
	return reflect(remainder ^ 0xFFFFFFFF, 32);
}

#include <stdio.h>

int main()
{
	initCRC32();
	uint32_t crc = sivaCRC32("123456789", 9);
	printf("%X\n", crc);
	return 0;
}

