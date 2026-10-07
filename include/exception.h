#ifndef DICT_EXCEPTION_H
#define DICT_EXCEPTION_H

enum DictionaryException {
  NullDictionaryException = 1,
  NullDictionaryElementException = 2,
  ArgumentException = 3,
  MemoryOverflowException = 4,
  InvalidStateException = 5,
  IOException = 6,
  NullHashFunctionException = 7,
  UnknownException = 100,
};

void eprint(enum DictionaryException ex);

#endif //DICT_EXCEPTION_H