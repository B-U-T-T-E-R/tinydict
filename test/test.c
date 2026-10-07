#include "../include/dict.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple test macro: if the condition is false, print an error and exit
#define ASSERT_TEST(cond, message) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s (Line %d)\n", message, __LINE__); \
            exit(1); \
        } \
    } while (0)

void Test_Initialization_And_Exceptions() {
    Dictionary *dict = malloc(DICTIONARY_SIZE);

    // 1. Test successful initialization
    int32_t res = InitializeDictionary(dict);
    ASSERT_TEST(res == 0, "Dictionary initialization failed");
    ASSERT_TEST(GetCount(dict) == 0, "New dictionary count must be 0");

    // 2. Test safety protection (ChangeHashFunction after data addition)
    char *key = "test_key";
    char *value = "test_value";

    res = SetValue(dict, key, value, strlen(key) + 1, strlen(value) + 1);
    ASSERT_TEST(res == 0, "SetValue failed");
    ASSERT_TEST(GetCount(dict) == 1, "Count must be 1 after insertion");

    // Try to change the hash function when the dictionary contains data
    res = ChangeHashFunction(dict, MurmurHash3_x64_128);
    // Should return InvalidStateException (code 5)
    ASSERT_TEST(res == 5, "ChangeHashFunction should fail with InvalidStateException if dict has data");

    // Try to change the seed when the dictionary contains data
    res = ChangeSeedForHash(dict, 12345);
    ASSERT_TEST(res == 5, "ChangeSeedForHash should fail with InvalidStateException if dict has data");

    DestroyDictionary(dict);
    printf("Test_Initialization_And_Exceptions: PASSED\n");
}

void Test_Keyless_Behavior_And_Clear() {
    Dictionary *dict = malloc(DICTIONARY_SIZE);
    InitializeDictionary(dict);

    char *key1 = "apple";
    char *val1 = "red";
    char *key2 = "banana";
    char *val2 = "yellow";

    SetValue(dict, key1, val1, strlen(key1) + 1, strlen(val1) + 1);
    SetValue(dict, key2, val2, strlen(key2) + 1, strlen(val2) + 1);

    // ContainsKey test
    ASSERT_TEST(ContainsKey(dict, key1, strlen(key1) + 1) == 0, "ContainsKey failed to find key1");
    ASSERT_TEST(ContainsKey(dict, "grape", 6) == -1, "ContainsKey found non-existent key");

    // TryGetValue test
    void *out_val = NULL;
    size_t out_size = 0;
    int32_t res = TryGetValue(dict, key2, strlen(key2) + 1, &out_val, &out_size);

    ASSERT_TEST(res == 0, "TryGetValue failed");
    ASSERT_TEST(out_size == strlen(val2) + 1, "TryGetValue returned wrong size");
    ASSERT_TEST(strcmp((char*)out_val, val2) == 0, "TryGetValue returned wrong value");

    free(out_val); // Memory allocated inside TryGetValue, cleaning it up

    // ClearDictionary test
    ClearDictionary(dict);
    ASSERT_TEST(GetCount(dict) == 0, "Count must be 0 after ClearDictionary");

    // After Clear the dictionary is empty, so changing hash/seed IS ALLOWED AGAIN
    res = ChangeSeedForHash(dict, 42);
    ASSERT_TEST(res == 0, "ChangeSeedForHash should succeed after ClearDictionary");

    DestroyDictionary(dict);
    printf("Test_Keyless_Behavior_And_Clear: PASSED\n");
}

int main() {
    printf("Running Tiny Dict tests...\n\n");

    Dictionary *dict = malloc(DICTIONARY_SIZE);
    Test_Initialization_And_Exceptions();
    Test_Keyless_Behavior_And_Clear();

    printf("\nAll tests successfully PASSED!\n");
    return 0;
}