#include "dict.h"

#include "hash.h"
#include "exception.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const uint64_t DICTIONARY_SIZE = 40;
const uint64_t DICTIONARY_ELEMENT_SIZE = 32;
static const uint64_t DEFAULT_CAPACITY = 10;

#define MAX_LEN_TEMP_PATH 32

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define LITTLE_ENDIAN 1
#elif defined(_MSC_VER)
#define LITTLE_ENDIAN 1
#else
#define LITTLE_ENDIAN 0
#endif

static inline uint32_t swap_uint32_t  (uint32_t value) {
  return ((value & 0x000000FFu) << 24)|
         ((value & 0x0000FF00u) <<  8)|
         ((value & 0x00FF0000u) >>  8)|
         ((value & 0xFF000000u) >> 24);

}
static inline uint64_t swap_uint64_t  (uint64_t value) {
  return ((value & 0x00000000000000FFull) << 56) |
         ((value & 0x000000000000FF00ull) << 40) |
         ((value & 0x0000000000FF0000ull) << 24) |
         ((value & 0x00000000FF000000ull) << 8)  |
         ((value & 0x000000FF00000000ull) >> 8)  |
         ((value & 0x0000FF0000000000ull) >> 24) |
         ((value & 0x00FF000000000000ull) >> 40) |
         ((value & 0xFF00000000000000ull) >> 56);
}
static inline __uint128_t swap_uint128_t (const __uint128_t value) {
  const uint64_t high = (uint64_t)(value >> 64);
  const uint64_t low  = (uint64_t)(value);

  return ((__uint128_t)swap_uint64_t(low) << 64 | swap_uint64_t(high));
}

struct DictionaryElement {
  __uint128_t key;
  void *value;
  uint64_t size;
};

struct Dictionary {
  uint64_t length;
  uint64_t capacity;
  uint64_t seed;
  DictionaryElement *elements;

  __uint128_t (*GetHashCode)(const void *key, uint64_t len, uint64_t seed);
};

static int32_t InitializeDictionaryInternal(Dictionary *dict, const uint64_t capacity) {
  if (dict == NULL) {
    eprint(NullDictionaryException);
    return NullDictionaryException;
  }
  if (capacity <= 0) {
    eprint(ArgumentException);
    return ArgumentException;
  }
  void *ptr = malloc(capacity * sizeof(DictionaryElement));
  if (ptr == NULL) {
    eprint(MemoryOverflowException);
    return MemoryOverflowException;
  }

  dict->elements = ptr;
  dict->capacity = capacity;
  dict->length = 0;
  for (uint64_t i = 0; i < capacity; i++) {
    dict->elements[i].key = 0;
    dict->elements[i].value = NULL;
    dict->elements[i].size = 0;
  }
  dict->GetHashCode = MurmurHash3_x64_128;
  dict->seed = 0;
  return 0;
}

int32_t InitializeDictionary(Dictionary *dict) {
  return InitializeDictionaryInternal(dict, DEFAULT_CAPACITY);
}

int32_t InitializeDictionaryWithCapacity(Dictionary *dict, const uint64_t capacity) {
  return InitializeDictionaryInternal(dict, capacity);
}

void DestroyDictionary(Dictionary *dict) {
  if (dict == NULL) {
    eprint(NullDictionaryException);
    return;
  }

  for (uint64_t i = 0; i < dict->capacity; i++) {
    free(dict->elements[i].value);
    dict->elements[i].value = NULL;
    dict->elements[i].key = 0;
    dict->elements[i].size = 0;
  }
  dict->GetHashCode = NULL;
  dict->seed = 0;
  free(dict->elements);
  dict->elements = NULL;
}

static int32_t ReallocateDictionaryInternal(Dictionary *dict, uint64_t capacity) {
  if (dict == NULL) {
    eprint(NullDictionaryException);
    return NullDictionaryException;
  }
  if (capacity < dict->capacity) {
    eprint(InvalidStateException);
    return InvalidStateException;
  }
  if (capacity == 0) {
    eprint(ArgumentException);
    return ArgumentException;
  }

  uint64_t new_size = 2 * dict->capacity;
  if (capacity > 0) {
    new_size = capacity;
  }
  DictionaryElement *ptr = realloc(dict->elements, new_size * sizeof(DictionaryElement));
  if (ptr == NULL) {
    eprint(MemoryOverflowException);
    return MemoryOverflowException;
  }

  for (uint64_t i = dict->capacity; i < new_size; i++) {
    ptr[i].key = 0;
    ptr[i].size = 0;
    ptr[i].value = NULL;
  }

  dict->elements = ptr;
  dict->capacity = new_size;
  return 0;
}

static int32_t ReallocateDictionaryWithCapacityInternal(Dictionary *dict, uint64_t capacity) {
  return ReallocateDictionaryInternal(dict, capacity);
}

static int32_t ReallocateDictionaryWithoutCapacityInternal(Dictionary *dict) {
  return ReallocateDictionaryInternal(dict, 0);
}

static int32_t GetIndexInternal(const Dictionary *dict, const void *key, uint64_t sizeKey);

static int32_t CreateKeyValueInternal(Dictionary *dict, const void *key, const void *value, uint64_t sizeKey, uint64_t sizeValue) {
  if (dict == NULL) {
    eprint(NullDictionaryException);
    return NullDictionaryException;
  }
  if (dict->length == dict->capacity) {
    int32_t result = ReallocateDictionaryWithoutCapacityInternal(dict);
    if (result > 0) {
      eprint(result);
      return result;
    }
  }

  if (key == NULL) {
    eprint(ArgumentException);
    return ArgumentException;
  }
  if (sizeKey == 0) {
    eprint(ArgumentException);
    return ArgumentException;
  }
  if (dict->GetHashCode == NULL) {
    eprint(NullHashFunctionException);
    return NullHashFunctionException;
  }
  const __uint128_t hash_key = dict->GetHashCode(key, sizeKey, dict->seed);
  const int32_t index = GetIndexInternal(dict, key, sizeKey);

  if (index == INT32_MIN) {
    dict->elements[dict->length].key = hash_key;
    if (value != NULL) {
      dict->elements[dict->length].value = malloc(sizeValue);
      memcpy(dict->elements[dict->length].value, value, sizeValue);
      dict->elements[dict->length].size = sizeValue;
    } else {
      dict->elements[dict->length].size = 0;
    }
    dict->length++;

    return 0;
  }

  if (index <= 0) {
    const int32_t tempIndex = -index;
    if (dict->elements[tempIndex].value != NULL) {
      free(dict->elements[tempIndex].value);
      dict->elements[tempIndex].value = NULL;
    }
    if (value == NULL) {
      dict->elements[tempIndex].size = 0;
    } else {
      dict->elements[tempIndex].size = sizeValue;
      dict->elements[tempIndex].value = malloc(sizeValue);
      if (dict->elements[tempIndex].value == NULL) {
        eprint(MemoryOverflowException);
        return MemoryOverflowException;
      }
      memcpy(dict->elements[tempIndex].value, value, sizeValue);
    }
    return 0;
  } else {
    eprint(index);
    return index;
  }
}

int32_t CreateKey(Dictionary *dict, const void *key, uint64_t sizeKey) {
  return CreateKeyValueInternal(dict, key, NULL, sizeKey, 0);
}

int32_t SetValue(Dictionary *dict, const void *key, const void *value, uint64_t sizeKey, uint64_t sizeValue) {
  return CreateKeyValueInternal(dict, key, value, sizeKey, sizeValue);
}

static const void *GetValueInternal(const Dictionary *dict, const void *key, uint64_t sizeKey) {
  if (dict == NULL) {
    eprint(NullDictionaryException);
    return NULL;
  }
  if (dict->length == 0) {
    eprint(ArgumentException);
    return NULL;
  }
  const int32_t index = GetIndexInternal(dict, key, sizeKey);

  if (index > 0 || index == INT32_MIN) {
    return NULL;
  }

  const void *const value = dict->elements[-index].value;

  return value;
}

const void *GetValue(const Dictionary *dict, const void *key, uint64_t sizeKey) {
  return GetValueInternal(dict, key, sizeKey);
}

void *GetCopyValue(const Dictionary *dict, const void *key, uint64_t sizeKey) {
  const void *ptr = GetValueInternal(dict, key, sizeKey);
  if (ptr == NULL) return NULL;
  const int32_t index = GetIndexInternal(dict, key, sizeKey);

  if (index > 0 || index == INT32_MIN) {
    return NULL;
  }

  const uint64_t size = dict->elements[-index].size;
  void *mem = malloc(size);
  if (mem == NULL) {
    eprint(MemoryOverflowException);
    return NULL;
  }
  memcpy(mem, ptr, size);

  return mem;
}

static int32_t GetIndexInternal(const Dictionary *dict, const void *key, uint64_t sizeKey) {
  if (dict == NULL) {
    eprint(NullDictionaryException);
    return NullDictionaryException;
  }
  if (dict->elements == NULL) {
    eprint(NullDictionaryElementException);
    return NullDictionaryElementException;
  }
  if (dict->length == 0) {
    return INT32_MIN;
  }

  if (dict->GetHashCode == NULL) {
    eprint(NullHashFunctionException);
    return NullHashFunctionException;
  }
  const __uint128_t hash_key = dict->GetHashCode(key, sizeKey, dict->seed);

  for (uint64_t i = 0; i < dict->length; i++) {
    if (dict->elements[i].key == hash_key) {
      return -(int32_t)i;
    }
  }

  return INT32_MIN;
}

int32_t ContainsKey(const Dictionary *dict, const void *key, const uint64_t sizeKey) {
  if (dict == NULL) {
    eprint(NullDictionaryException);
    return NullDictionaryException;
  }
  if (dict->elements == NULL) {
    eprint(NullDictionaryElementException);
    return NullDictionaryElementException;
  }
  if (dict->length == 0) {
    return 0;
  }
  if (dict->GetHashCode == NULL) {
    eprint(NullHashFunctionException);
    return NullHashFunctionException;
  }

  const __uint128_t hash_key = dict->GetHashCode(key, sizeKey, dict->seed);

  for (uint64_t i = 0; i < dict->length; i++) {
    if (dict->elements[i].key == hash_key) {
      return 0;
    }
  }

  return -1;
}

int32_t ContainsValue(const Dictionary *dict, const void *value, const uint64_t sizeValue) {
  if (dict == NULL) {
    eprint(NullDictionaryException);
    return NullDictionaryException;
  }
  if (dict->elements == NULL) {
    eprint(NullDictionaryElementException);
    return NullDictionaryElementException;
  }
  if (dict->length == 0) {
    return -1;
  }
  for (uint64_t i = 0; i < dict->length; i++) {
    if (dict->elements[i].key == 0) continue;
    const void *dictValue = dict->elements[i].value;
    if (!memcmp(dictValue, value, sizeValue)) {
      return 0;
    }
  }

  return -1;
}

int32_t TryGetValue(const Dictionary *dict, const void *key, uint64_t sizeKey, void **outValue, uint64_t *outSizeValue) {
  if (dict == NULL) {
    eprint(NullDictionaryException);
    return NullDictionaryException;
  }
  if (dict->elements == NULL) {
    eprint(NullDictionaryElementException);
    return NullDictionaryElementException;
  }
  if (outSizeValue == NULL) {
    eprint(ArgumentException);
    return ArgumentException;
  }

  const int32_t index = GetIndexInternal(dict, key, sizeKey);

  if (index > 0 || index == INT32_MIN) {
    *outValue = NULL;
    *outSizeValue = 0;
    return -1;
  }

  *outSizeValue = dict->elements[-index].size;
  *outValue = malloc(dict->elements[-index].size);
  if (*outValue == NULL) {
    eprint(MemoryOverflowException);
    return MemoryOverflowException;
  }
  memcpy(*outValue, dict->elements[-index].value, dict->elements[-index].size);
  *outSizeValue = dict->elements[-index].size;
  return 0;
}

void ClearDictionary(Dictionary *dict) {
  if (dict == NULL) {
    eprint(NullDictionaryException);
    return;
  }
  if (dict->elements == NULL) {
    eprint(NullDictionaryElementException);
    return;
  }
  for (uint64_t i = 0; i < dict->length; i++) {
    dict->elements[i].key = 0;
    dict->elements[i].size = 0;
    if (dict->elements[i].value == NULL) continue;
    free(dict->elements[i].value);
    dict->elements[i].value = NULL;
  }
  dict->length = 0;
}

__uint128_t *GetKeys(const Dictionary *dict, uint64_t *outCount) {
  if (dict == NULL) {
    eprint(NullDictionaryException);
    return NULL;
  }
  if (dict->elements == NULL) {
    eprint(NullDictionaryElementException);
    return NULL;
  }
  if (outCount == NULL) {
    eprint(ArgumentException);
    return NULL;
  }
  *outCount = dict->length;
  __uint128_t *values = (__uint128_t *) malloc(dict->length * sizeof(__uint128_t));
  if (values == NULL) {
    eprint(MemoryOverflowException);
    return NULL;
  }
  for (uint64_t i = 0; i < dict->length; i++) {
    values[i] = dict->elements[i].key;
  }
  return values;
}

void **GetValues(const Dictionary *dict, uint64_t *outCount) {
  if (outCount == NULL) {
    eprint(ArgumentException);
    return NULL;
  }
  if (dict == NULL) {
    eprint(NullDictionaryException);
    *outCount = 0;
    return NULL;
  }
  if (dict->elements == NULL) {
    eprint(NullDictionaryElementException);
    *outCount = 0;
    return NULL;
  }
  *outCount = dict->length;
  void **values = malloc(dict->length * sizeof(void *));
  if (values == NULL) {
    eprint(MemoryOverflowException);
    *outCount = 0;
    return NULL;
  }

  for (uint64_t i = 0; i < dict->length; i++) {
    values[i] = malloc(dict->elements[i].size);
    if (values[i] == NULL) {
      eprint(MemoryOverflowException);
      *outCount = 0;
      for (uint64_t j = 0; j < i; j++) {
        free(values[j]);
      }
      free(values);
      return NULL;
    }
    memcpy(values[i], dict->elements[i].value, dict->elements[i].size);
  }

  return values;
}

int32_t EnsureCapacity(Dictionary *dict, const uint64_t capacity) {
  if (dict == NULL) {
    eprint(NullDictionaryException);
    return NullDictionaryException;
  }
  if (dict->elements == NULL) {
    eprint(NullDictionaryElementException);
    return NullDictionaryElementException;
  }
  int32_t should_realloc = dict->capacity - dict->length - capacity > 0;
  if (should_realloc) {
    return 0;
  }
  return ReallocateDictionaryWithCapacityInternal(dict, capacity);
}

int32_t GetCount(const Dictionary *dict) {
  if (dict == NULL) {
    eprint(NullDictionaryException);
    return -NullDictionaryException;
  }
  return (int32_t)dict->length;
}

int32_t TryAdd(Dictionary *dict, const void *key, const void *value, uint64_t sizeKey, uint64_t sizeValue) {
  if (dict == NULL) {
    eprint(NullDictionaryException);
    return NullDictionaryException;
  }
  if (dict->elements == NULL) {
    eprint(NullDictionaryElementException);
    return NullDictionaryElementException;
  }
  const int32_t index = GetIndexInternal(dict, key, sizeKey);
  if (index <= 0 && index != INT32_MIN) {
    return -1;
  }

  return CreateKeyValueInternal(dict, key, value, sizeKey, sizeValue);;
}

int32_t EqualsDictionary(const Dictionary *dictA, const struct Dictionary *dictB) {
  if (dictA == NULL || dictB == NULL) {
    eprint(NullDictionaryException);
    return NullDictionaryException;
  }
  if (dictA->elements == NULL || dictB->elements == NULL) {
    eprint(NullDictionaryElementException);
    return NullDictionaryElementException;
  }
  if (dictA->length != dictB->length || dictA->length != dictB->capacity) {
    return -1;
  }
  for (uint64_t i = 0; i < dictA->length; i++) {
    if (dictA->elements[i].key != dictB->elements[i].key || dictA->elements[i].size != dictB->elements[i].size) {
      return -1;
    }
    if (memcmp(dictA->elements[i].value, dictB->elements[i].value, dictA->elements[i].size) != 0) { return -1; }
  }

  return 0;
}

int32_t ChangeHashFunction(Dictionary *dict,
                           __uint128_t (*newHashCode)(const void *key, uint64_t sizeKey, uint64_t seed)) {
  if (newHashCode == NULL) {
    eprint(ArgumentException);
    return ArgumentException;
  }
  if (dict == NULL) {
    eprint(NullDictionaryException);
    return NullDictionaryException;
  }
  if (dict->length > 0) {
    eprint(InvalidStateException);
    return InvalidStateException;
  }
  dict->GetHashCode = newHashCode;
  return 0;
}

int32_t ChangeHashFunctionUnsafe(Dictionary *dict, __uint128_t (*newHashCode)(const void *key, uint64_t sizeKey, uint64_t seed)) {
  if (newHashCode == NULL) {
    eprint(ArgumentException);
    return ArgumentException;
  }
  if (dict == NULL) {
    eprint(NullDictionaryException);
    return NullDictionaryException;
  }
  dict->GetHashCode = newHashCode;
  return 0;
}

int32_t ChangeSeedForHash(Dictionary *dict, const uint64_t newSeed) {
  if (dict == NULL) {
    eprint(NullDictionaryException);
    return NullDictionaryException;
  }
  if (dict->length > 0) {
    eprint(InvalidStateException);
    return InvalidStateException;
  }
  dict->seed = newSeed;
  return 0;
}

static const uint32_t magic = 0x594E5954;
static const uint32_t flags = 0x454c5300;
int32_t Serialization(const Dictionary *dict, const char *path) {
  if (dict == NULL) {
    eprint(NullDictionaryException);
    return NullDictionaryException;
  }
  if (dict->elements == NULL) {
    eprint(NullDictionaryElementException);
    return NullDictionaryElementException;
  }

  char *endian;

#if LITTLE_ENDIAN == 1
  endian = "LE";
#else
  endian = "BE";
#endif

  char *final_path = NULL;
  if (path == NULL) {
    const time_t t = time(NULL);
    const struct tm *tm = localtime(&t);
    final_path = malloc(MAX_LEN_TEMP_PATH);
    if (final_path == NULL) {
      eprint(MemoryOverflowException);
      return MemoryOverflowException;
    }
    uint64_t len = strftime(final_path, MAX_LEN_TEMP_PATH, "%d%m%y%H%M%S", tm);
    strcpy(final_path + len, ".bin");
  }
  else {
    const char *prev_ptr = NULL;
    const char *ptr = strchr(path, '.');
    while (ptr != NULL) {
      prev_ptr = ptr;
      ptr = strchr(ptr, '.');
    }

    if (prev_ptr == NULL) {
      final_path = malloc(strlen(path) + 5);
      if (final_path == NULL) {
        eprint(MemoryOverflowException);
        return MemoryOverflowException;
      }
      strcpy(final_path, path);
      strcpy(final_path + strlen(path), ".bin");
    }
    else {
      const uint64_t index = prev_ptr - path;
      const uint64_t len = strlen(path);
      int32_t should_add_path = 0;
      for (uint64_t i = index; i < len; i++) {
        if (path[i] == '\\' || path[i] == '/') {
          should_add_path = 1;
          break;
        }
      }
      if (should_add_path) {
        final_path = malloc(len + 5);
        if (final_path == NULL) {
          eprint(MemoryOverflowException);
          return MemoryOverflowException;
        }
        strcpy(final_path, path);
        strcpy(final_path + len, ".bin");
      }
      else {
        final_path = strdup(path);
      }
    }
  }

  FILE *fp = fopen(final_path, "wb");
  if (fp == NULL) {
    free(final_path);
    eprint(IOException);
    return IOException;
  }

  fwrite(&magic, sizeof(uint32_t), 1, fp);
  fwrite(endian, 1, 2, fp);
  fwrite(&dict->seed, sizeof(uint64_t), 1, fp);
  fwrite(&dict->capacity, sizeof(uint64_t), 1, fp);
  fwrite(&dict->length, sizeof(uint64_t), 1, fp);
  fwrite(&flags, sizeof(uint32_t), 1, fp);

  for (uint64_t i = 0; i < dict->length; i++) {
    fwrite(&dict->elements[i].key, sizeof(__uint128_t), 1, fp);
    fwrite(&dict->elements[i].size, sizeof(uint64_t), 1, fp);
    fwrite(dict->elements[i].value, 1, dict->elements[i].size, fp);
  }
  fclose(fp);

  free(final_path);

  return 0;
}
int32_t Deserialization(Dictionary *dict, const char *path) {
  if (dict == NULL) {
    eprint(NullDictionaryException);
    return NullDictionaryException;
  }

  if (path == NULL) {
    eprint(ArgumentException);
    return ArgumentException;
  }

  FILE *fp = fopen(path, "rb");
  if (fp == NULL) {
    eprint(IOException);
    return IOException;
  }

  uint32_t temp_magic = 0;
  fread(&temp_magic, sizeof(uint32_t), 1, fp);
  if (temp_magic != magic) {
    eprint(InvalidStateException);
    fclose(fp);
    return InvalidStateException;
  }
  uint8_t endian_sys = 0;
#if LITTLE_ENDIAN == 1
  endian_sys = 1;
#else
  endian_sys = 2;
#endif

  uint8_t endian_file = 0;
  char *endian_str = malloc(3);
  endian_str[2] = '\0';
  fread(endian_str, 1, 2, fp);
  if (strcmp(endian_str, "LE") == 0) {
    endian_file = 1;
  }
  else if (strcmp(endian_str, "BE") == 0) {
    endian_file = 2;
  }
  else {
    eprint(InvalidStateException);
    free(endian_str);
    fclose(fp);
  }
  free(endian_str);
  fread(&dict->seed, sizeof(uint64_t), 1, fp);
  fread(&dict->capacity, sizeof(uint64_t), 1, fp);
  fread(&dict->length, sizeof(uint64_t), 1, fp);
  uint32_t flag = 0;

  fread(&flag, sizeof(uint32_t), 1, fp);
  if (endian_file != endian_sys) {
    dict->seed = swap_uint64_t(dict->seed);
    dict->capacity = swap_uint64_t(dict->capacity);
    dict->length = swap_uint64_t(dict->length);
    flag = swap_uint32_t(flag);
  }
  if (flag != flags) {
    eprint(InvalidStateException);
    fclose(fp);
    return InvalidStateException;
  }
  dict->elements = malloc(DICTIONARY_ELEMENT_SIZE * dict->capacity);

  void **values = malloc(dict->capacity * sizeof(void *));
  if (values == NULL) {
    dict->seed = 0;
    dict->capacity = 0;
    dict->length = 0;
    free(dict->elements);
    dict->elements = NULL;
    fclose(fp);
    printf("%d\n", __LINE__);
    eprint(MemoryOverflowException);
    return MemoryOverflowException;
  }
  for (uint64_t i = 0; i < dict->length; i++) {
    __uint128_t key = 0;
    uint64_t size = 0;

    fread(&key, sizeof(__uint128_t), 1, fp);
    fread(&size, sizeof(uint64_t), 1, fp);

    if (endian_file != endian_sys) {
      key = swap_uint128_t(key);
      size = swap_uint64_t(size);
    }

    if (size > 0) {
      values[i] = malloc(size);
      if (values[i] == NULL) {
        for (uint64_t j = 0; j < i; j++) free(values[j]);
        free(values);
        dict->seed = 0;
        dict->capacity = 0;
        dict->length = 0;
        free(dict->elements);
        dict->elements = NULL;
        fclose(fp);

        printf("%zu\t%d\n", size, __LINE__);
        eprint(MemoryOverflowException);
        return MemoryOverflowException;
      }
      fread(values[i], 1, size, fp);
    }
    else {
      values[i] = NULL;
    }

    dict->elements[i].key = key;
    dict->elements[i].size = size;
    dict->elements[i].value = values[i];
  }
  for (uint64_t i = dict->length; i < dict->capacity; i++) {
    dict->elements[i].key = 0;
    dict->elements[i].size = 0;
    dict->elements[i].value = NULL;
  }

  dict->GetHashCode = MurmurHash3_x64_128;

  free(values);
  values = NULL;

  fclose(fp);
  return 0;
}

static uint8_t PercentageMove = 25;

static int32_t MoveOrCreate(const Dictionary *dict, const void *key, uint64_t sizeKey, uint64_t sizeValue) {
  int32_t index = GetIndexInternal(dict, key, sizeKey);
  if (index == INT32_MIN) {
    return -1;
  }
  if (index > 0) {
    return index;
  }
  index = -index;
  const uint64_t sizeVal1 = dict->elements[index].size;

  if (sizeVal1 < sizeValue) return -1;
  if (sizeValue == 0) return -1;
  if (sizeVal1 * 100 / sizeValue <= PercentageMove) return 0;

  return -1;
}

int32_t Update(Dictionary *dict, const void *key, const void *value, uint64_t sizeKey, uint64_t sizeValue) {
  if (dict == NULL) {
    eprint(NullDictionaryException);
    return NullDictionaryException;
  }
  if (dict->elements == NULL) {
    eprint(NullDictionaryElementException);
    return NullDictionaryElementException;
  }
  if (key == NULL) {
    eprint(ArgumentException);
    return ArgumentException;
  }

  int32_t index = GetIndexInternal(dict, key, sizeKey);

  if (index == INT32_MIN) {
    return -1;
  }
  if (index > 0) {
    return index;
  }

  index = -index;


  if (value == NULL) {
    free(dict->elements[index].value);
    dict->elements[index].value = NULL;
    dict->elements[index].size = 0;
  }
  else {
    int32_t result = MoveOrCreate(dict, key, sizeKey, sizeValue);
    if (result > 0) {
      return result;
    }
    if (result == -1) {
      free(dict->elements[index].value);
      dict->elements[index].value = malloc(sizeValue);
      if (dict->elements[index].value == NULL) {
        eprint(MemoryOverflowException);
        return MemoryOverflowException;
      }
    }
    memcpy(dict->elements[index].value, value, sizeValue);
    dict->elements[index].size = sizeValue;
  }

  return 0;
}

int32_t TryAddOrUpdate(Dictionary *dict, const void *key, const void *value, uint64_t sizeKey, uint64_t sizeValue) {
  if (dict == NULL) {
    eprint(NullDictionaryException);
    return NullDictionaryException;
  }
  if (dict->elements == NULL) {
    eprint(NullDictionaryElementException);
    return NullDictionaryElementException;
  }
  if (key == NULL) {
    eprint(ArgumentException);
    return ArgumentException;
  }
  int32_t index = GetIndexInternal(dict, key, sizeKey);
  if (index > 0) {
    return index;
  }

  if (dict->length == dict->capacity) {
    int32_t result = ReallocateDictionaryWithoutCapacityInternal(dict);
    if (result > 0) return result;
  }

  if (index == INT32_MIN) {
    int32_t result = CreateKeyValueInternal(dict, key, value, sizeKey, sizeValue);
    return result == 0 ? -1 : result;
  }
  else {
    int32_t result = Update(dict, key, value, sizeKey, sizeValue);
    return result;
  }
}

int32_t GetCapacityLeft(const Dictionary *dict) {
  if (dict == NULL) {
    eprint(NullDictionaryException);
    return -NullDictionaryException;
  }
  if (dict->elements == NULL) {
    eprint(NullDictionaryElementException);
    return -NullDictionaryElementException;
  }

  return (int32_t)(dict->capacity - dict->length);
}

int32_t ChangePercentageMoveOrCreate(const uint8_t newPercentage) {
  if (newPercentage > 50) return -1;
  PercentageMove = newPercentage;
  return 0;
}