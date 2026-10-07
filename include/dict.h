#ifndef DICT_DICT_H
#define DICT_DICT_H

#include <stdint.h>

typedef struct DictionaryElement DictionaryElement;
typedef struct Dictionary Dictionary;

extern const uint64_t DICTIONARY_SIZE;
extern const uint64_t DICTIONARY_ELEMENT_SIZE;

//Initializing a dictionary with default capacity.
//Returns 0 if successful
//Exceptions:
//  NullDictionaryException
//  ArgumentException
//  MemoryOverflowException
int32_t InitializeDictionary(Dictionary *dict);

//Initializing a dictionary with a specified capacity.
//Returns 0 if successful
//Exceptions:
//  NullDictionaryException
//  ArgumentException
//  MemoryOverflowException
int32_t InitializeDictionaryWithCapacity(Dictionary *dict, uint64_t capacity);

//Freeing a dictionary from RAM.
void DestroyDictionary(Dictionary *dict);

//Creates a key in the dictionary.
//Returns 0 if successful
//Exceptions:
//  NullDictionaryException
//  ArgumentException
//  MemoryOverflowException
//  InvalidStateException
//  NullHashFunctionException
int32_t CreateKey(Dictionary *dict, const void *key, uint64_t sizeKey);

//Sets a value by key (not recommended;
//if a value already exists for the key,
//it will be deleted and a new one assigned)
//Return 0 if successful
//Exceptions:
//  NullDictionaryException
//  ArgumentException
//  MemoryOverflowException
//  InvalidStateException
//  NullHashFunctionException
int32_t SetValue(Dictionary *dict, const void *key, const void *value, uint64_t sizeKey, uint64_t sizeValue);

//Returns a constant value by key
//Returns NULL if the key does not exist
//Exceptions:
//  NullDictionaryException
//  ArgumentException
//  NullHashFunctionException
const void * GetValue(const Dictionary *dict, const void *key, uint64_t sizeKey);

//Returns a copy of the value by key (IMPORTANT: the value must be freed after use)
//Returns NULL if the key does not exist
//Exceptions:
//  NullDictionaryException
//  NullDictionaryElementException
//  ArgumentException
//  MemoryOverflowException
//  NullHashFunctionException
void * GetCopyValue(const Dictionary *dict, const void *key, uint64_t sizeKey);

//Checks if the key exists
//Returns 0 if the key does exist
//Exceptions:
//  NullDictionaryException
//  NullDictionaryElementException
//  NullHashFunctionException
int32_t ContainsKey(const Dictionary *dict, const void *key, uint64_t sizeKey);

//Checks if the value exists (regardless of the key)
//Returns 0 if the value does exist
//Exceptions:
//  NullDictionaryException
//  NullDictionaryElementException
int32_t ContainsValue(const Dictionary *dict, const void *value, uint64_t sizeValue); // First occurrence of the value

// Safely retrieves the value by key into outValue.
// Returns 0 on success
// Note: Pass outValue as NULL;
// the function allocates memory from scratch to avoid buffer underflow.
//Exceptions:
//  NullDictionaryException
//  NullDictionaryElementException
//  ArgumentException
//  MemoryOverflowException
//  NullHashFunctionException
int32_t TryGetValue(const Dictionary *dict, const void *key, uint64_t sizeKey, void **outValue, uint64_t *outSizeValue);

//Clears the dictionary by removing all
//elements and freeing their memory,
//while maintaining the current capacity.
//Exceptions:
//  NullDictionaryException
//  ArgumentException
//  MemoryOverflowException
void ClearDictionary(Dictionary *dict);

//Allocates and returns an array of all key hashes;
//stores the total count in outCount
//Exceptions:
//  NullDictionaryException
//  NullDictionaryElementException
//  ArgumentException
//  MemoryOverflowException
__uint128_t* GetKeys(const Dictionary *dict, uint64_t *outCount);

//Allocates and returns an array of pointers to all values;
//stores the total count in outCount
//Exceptions:
//  NullDictionaryException
//  NullDictionaryElementException
//  ArgumentException
//  MemoryOverflowException
void** GetValues(const Dictionary *dict, uint64_t *outCount);

//Ensures that the dictionary can hold
//the specified number of elements
//without memory reallocation.
//Returns 0 if the dictionary can accommodate the elements without memory reallocation.
//Exceptions:
//  NullDictionaryException
//  NullDictionaryElementException
//  InvalidStateException
//  ArgumentException
//  MemoryOverflowException
int32_t EnsureCapacity(Dictionary *dict, uint64_t capacity);

//Returns the number of elements in the dictionary.
//Exceptions:
//  -NullDictionaryException
//  -NullDictionaryElementException
int32_t GetCount(const Dictionary *dict);

//Tries to add a key-value pair;
//returns -1 if the key already exists,
//and 0 upon successful addition.
//Exceptions:
//  NullDictionaryException
//  NullDictionaryElementException
//  ArgumentException
//  MemoryOverflowException
//  InvalidStateException
//  NullHashFunctionException
int32_t TryAdd(Dictionary *dict, const void *key, const void *value, uint64_t sizeKey, uint64_t sizeValue);

//Checks if the structure and capacity match another dictionary.
//Returns 0 if they are equal
//Exceptions:
//  NullDictionaryException
//  NullDictionaryElementException
int32_t EqualsDictionary(const Dictionary *dictA, const Dictionary *dictB);

//Updates the hash function unless the dictionary is not empty
//(has at least one key-value pair)
//Returns 0 if successful
//Exceptions:
//  ArgumentException
//  NullDictionaryException
//  InvalidStateException
int32_t ChangeHashFunction(Dictionary *dict, __uint128_t (*newHashCode)(const void *key, uint64_t sizeKey, uint64_t seed));

//Updates the seed for hash function unless the dictionary is not empty
//(has at least one key-value pair)
//Returns 0 if successful
//Exceptions:
//  ArgumentException
//  NullDictionaryException
//  InvalidStateException
int32_t ChangeSeedForHash(Dictionary *dict, uint64_t newSeed);

//Changes the hash function in a dictionary;
//not recommended, as there is no re-hashing function (UNSAFE).
//Returns 0 if successful
//Exceptions:
//  ArgumentException
//  NullDictionaryException
int32_t ChangeHashFunctionUnsafe(Dictionary *dict, __uint128_t (*newHashCode)(const void *key, uint64_t sizeKey, uint64_t seed));

//Serializes the dictionary;
//if NULL is passed as a path,
//it will be saved using the mask 'DDMMYYhhmmss.bin'
//Returns 0 if successful
//Exceptions:
//  NullDictionaryException
//  NullDictionaryElementException
//  MemoryOverflowException
//  IOException
int32_t Serialization(const Dictionary *dict, const char *path);

//Deserializes the dictionary;
//all elements will be allocated on the heap.
//IMPORTANT: DO NOT ALLOCATE IN ADVANCE.
//After deserialization, use ChangeHashFunctionUnsafe if you used a
//different hash function; otherwise, the Default Hash Function will be used.
//Returns 0 if successful
//Exceptions:
//  NullDictionaryException
//  ArgumentException
//  IOException
//  InvalidStateException
//  MemoryOverflowException
int32_t Deserialization(Dictionary *dict, const char *path);

//Updates the value by key.
//Returns -1 if the key does not exist;
//Returns 0 if successful.
//Exceptions:
//  NullDictionaryException
//  NullDictionaryElementException
//  ArgumentException
//  MemoryOverflowException
//  NullHashFunctionException
int32_t Update(Dictionary *dict, const void *key, const void *value, uint64_t sizeKey, uint64_t sizeValue);

//Updates the value by key if it already exists;
//otherwise, creates it.
//Returns -1 if a new entry was created.
//Returns 0 if the value was updated.
//Exceptions:
//  NullDictionaryException
//  NullDictionaryElementException
//  ArgumentException
//  InvalidStateException
//  MemoryOverflowException
//  NullHashFunctionException
int32_t TryAddOrUpdate(Dictionary *dict, const void *key, const void *value, uint64_t sizeKey, uint64_t sizeValue);

//Returns the number of remaining free entries in the dictionary
//Exceptions:
//  -NullDictionaryException
//  -NullDictionaryElementException
int32_t GetCapacityLeft(const Dictionary *dict);

//Changes the fill ratio (load factor),
//which affects the Update and TryAddOrUpdate functions.
//When the incoming data buffer is smaller than
//the existing one, it clears the memory area and writes
//the new data instead of triggering frequent malloc calls.
//The maximum allowed ratio is 50%.
//Returns 0 if successful.
int32_t ChangePercentageMoveOrCreate(uint8_t newPercentage);

#endif //DICT_DICT_H