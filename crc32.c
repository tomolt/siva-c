#include <stdint.h>

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
		uint32_t remainder = byte;
		for (int i = 8; i > 0; --i) {
			int botbit = remainder & 1;
			remainder >>= 1;
			if (botbit) {
				remainder ^= reflect(0x04C11DB7, 32);
			}
		}
		LUT[byte] = remainder;
	}
}

uint32_t sivaCRC32(uint8_t message[], uint64_t count)
{
	uint32_t remainder = 0xFFFFFFFF;	
	for (int i = 0; i < count; ++i) {
		uint32_t botbyte = message[i] ^ (remainder & 0xFF);
		remainder = LUT[botbyte] ^ (remainder >> 8);
	}
	return remainder ^ 0xFFFFFFFF;
}

#include <stdio.h>

int main()
{
	initCRC32();
	uint32_t crc = sivaCRC32("123456789", 9);
	printf("%X\n", crc);
	return 0;
}

