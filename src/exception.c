#include "exception.h"

#include <stdio.h>

static const char *get_string_exception(const enum DictionaryException ex) {
  switch (ex) {
    case NullDictionaryException: return "NullDictionaryException";
    case NullDictionaryElementException: return "NullDictionaryElementException";
    case ArgumentException: return "ArgumentException";
    case MemoryOverflowException: return "MemoryOverflowException";
    case InvalidStateException: return "InvalidStateException";
    case UnknownException: default: return "UnknownException";
  }
}

void eprint(const enum DictionaryException ex) {
  fprintf(stderr, "Err. %s. Error code: %d\n", get_string_exception(ex), ex);
  fflush(stderr);
}
