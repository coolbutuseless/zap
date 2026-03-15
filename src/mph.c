


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "mph.h"
#include "chibihash.h"



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Destroy an mph and all allocated memory
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void mph_destroy(mph_t *mph) {
  
  if (mph == NULL) return;
  if (mph->bucket != NULL) {
    for (int i = 0; i < mph->capacity; i++) {
      free(mph->bucket[i].key);
    }
    free(mph->bucket);
  }
  free(mph);
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Initialize a hashmap
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mph_t *mph_init(size_t capacity) {
  if (capacity < 1) {
    return NULL;
  }
  
  mph_t *mph = calloc(1, sizeof(mph_t));
  if (mph == NULL) {
    return NULL;
  }
  mph->capacity    = capacity;
  mph->nitems = 0;
  
  mph->bucket = calloc(capacity, sizeof(bucket_t));
  if (mph->bucket == NULL) {
    return NULL;
  }
  
  return mph;
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Lookup a single key in the hashmap.
// Returns the 0-based index
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int32_t mph_get(mph_t *mph, uint8_t *key, size_t len) {
  const uint64_t hash = chibihash64(key, (ptrdiff_t)len, 0xdeadbeef);
  uint32_t idx = (uint32_t)(hash % mph->capacity);
  
  bucket_t bucket = mph->bucket[idx];
  
  while (bucket.key != NULL) {
    if (bucket.hash == hash && bucket.len  == len &&
        memcmp(key, bucket.key, bucket.len) == 0) {
      return bucket.value;
    }
    idx = (idx + 1) % mph->capacity;
    bucket = mph->bucket[idx];
  }
  
  return MPH_NOT_FOUND;
}  



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Add a key to the hashmap
// The value is implicitly set to the 0-based index of the item in the hashmap
// i.e. the first string added has a value of '0'
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool mph_set(mph_t *mph, uint8_t *key, size_t len) {
  // Hash key, and calculate the bucket
  const uint64_t hash = chibihash64(key, (ptrdiff_t)len, 0xdeadbeef);
  uint32_t idx  = (uint32_t)(hash % mph->capacity);
  const int32_t value = (int32_t)mph->nitems++;
  
  // Linear probing for an empty bucket
  while (mph->bucket[idx].key != NULL) {
    idx = (idx + 1) % mph->capacity;
  }
  
  // Add key to the hashmap
  mph->bucket[idx].value = value;
  mph->bucket[idx].hash  = hash;
  mph->bucket[idx].len   = len;
  mph->bucket[idx].key   = malloc(len + 1); // keep null-termintor
  if (mph->bucket[idx].key == NULL) {
    return false;
  }
  memcpy(mph->bucket[idx].key, key, len + 1); // keep null-terminator
  
  return true;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Lookup a single key in the hashmap, add it first if it is not present
// Returns the 0-based index, or -1 in case of error
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int32_t mph_get_set(mph_t *mph, uint8_t *key, size_t len) {
  const uint64_t hash = chibihash64(key, (ptrdiff_t)len, 0xdeadbeef);
  uint32_t idx = (uint32_t)(hash % mph->capacity);
  
  bucket_t bucket = mph->bucket[idx];
  
  // Search for existing match
  while (bucket.key != NULL) {
    if (bucket.hash == hash && bucket.len  == len &&
        memcmp(key, bucket.key, bucket.len) == 0) {
      return bucket.value;
    }
    idx = (idx + 1) % mph->capacity;
    bucket = mph->bucket[idx];
  }
  
  // If we get to here, a match has not been found,
  // and 'idx' is set to an appropriate empty slot
  const int32_t value = (int32_t)mph->nitems++;
  
  // Add key to the hashmap
  mph->bucket[idx].value = value;
  mph->bucket[idx].hash  = hash;
  mph->bucket[idx].len   = len;
  mph->bucket[idx].key   = malloc(len);
  if (mph->bucket[idx].key == NULL) {
    return MPH_ERROR;
  }
  memcpy(mph->bucket[idx].key, key, len);
  
  
  return value;
}  







