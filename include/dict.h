#ifndef DICT_DICT_H
#define DICT_DICT_H

#include "hash.h"

typedef struct DictionaryElement DictionaryElement;
typedef struct Dictionary Dictionary;

extern const size_t DICTIONARY_SIZE;
extern const size_t DICTIONARY_ELEMENT_SIZE;

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
int32_t InitializeDictionaryWithCapacity(Dictionary *dict, size_t capacity);
//Freeing a dictionary from RAM.
void DestroyDictionary(Dictionary *dict);

//Creates a key in the dictionary.
//Returns 0 if successful
//Exceptions:
//  NullDictionaryException
//  ArgumentException
//  MemoryOverflowException
//  InvalidStateException
int32_t CreateKey(Dictionary *dict, const void *key, size_t sizeKey);
//Sets a value by key (not recommended;
//if a value already exists for the key,
//it will be deleted and a new one assigned)
//Return 0 if successful
//Exceptions:
//  NullDictionaryException
//  ArgumentException
//  MemoryOverflowException
//  InvalidStateException
int32_t SetValue(Dictionary *dict, const void *key, const void *value, size_t sizeKey, size_t sizeValue);

//Returns a constant value by key
//Returns NULL if the key does not exist
//Exceptions:
//  NullDictionaryException
//  ArgumentException
const void * GetValue(const Dictionary *dict, const void *key, size_t sizeKey);
//Returns a copy of the value by key (IMPORTANT: the value must be freed after use)
//Returns NULL if the key does not exist
//Exceptions:
//  NullDictionaryException
//  NullDictionaryElementException
//  ArgumentException
//  MemoryOverflowException
void * GetCopyValue(const Dictionary *dict, const void *key, size_t sizeKey);

//Checks if the key exists
//Returns 0 if the key does exist
//Exceptions:
//  NullDictionaryException
//  NullDictionaryElementException
int32_t ContainsKey(const Dictionary *dict, const void *key, size_t sizeKey);
//Checks if the value exists (regardless of the key)
//Returns 0 if the values does exist
//Exceptions:
//  NullDictionaryException
//  NullDictionaryElementException
int32_t ContainsValue(const Dictionary *dict, const void *value, size_t sizeValue); // First occurrence of the value

// Safely retrieves the value by key into outValue.
// Returns 0 on success
// Note: Pass outValue as NULL;
// the function allocates memory from scratch to avoid buffer underflow.
//Exceptions:
//  NullDictionaryException
//  NullDictionaryElementException
//  ArgumentException
//  MemoryOverflowException
int32_t TryGetValue(const Dictionary *dict, const void *key, size_t sizeKey, void **outValue, size_t *outSizeValue);

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
__uint128_t* GetKeys(const Dictionary *dict, size_t *outCount);

//Allocates and returns an array of pointers to all values;
//stores the total count in outCount
//Exceptions:
//  NullDictionaryException
//  NullDictionaryElementException
//  ArgumentException
//  MemoryOverflowException
void** GetValues(const Dictionary *dict, size_t *outCount);

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
int32_t EnsureCapacity(Dictionary *dict, size_t capacity);

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
int32_t TryAdd(Dictionary *dict, const void *key, const void *value, size_t sizeKey, size_t sizeValue);

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
int32_t ChangeHashFunction(Dictionary *dict, __uint128_t (*newHashCode)(const void *key, size_t sizeKey, uint64_t seed));
//Updates the seed for hash function unless the dictionary is not empty
//(has at least one key-value pair)
//Returns 0 if successful
//Exceptions:
//  ArgumentException
//  NullDictionaryException
//  InvalidStateException
int32_t ChangeSeedForHash(Dictionary *dict, uint64_t newSeed);

#endif //DICT_DICT_H