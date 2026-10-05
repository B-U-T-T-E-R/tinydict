#ifndef DICT_HASH_H
#define DICT_HASH_H

#include <stdint.h>

extern __uint128_t MurmurHash3_x64_128(const void *key, size_t len, uint64_t seed);

#endif //DICT_HASH_H