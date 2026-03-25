
#pragma once

#define MPH_ERROR -2
#define MPH_NOT_FOUND -1

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Buckets
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  uint8_t *key;    // Array: pointer to the keys (hash retrains a copy of original key)
  size_t   len;    // Array: the lengths of each key
  uint64_t hash;   // Array: hash of the key
  int32_t  value;  // Array: integer value for each key i.e. index of insertion order
} bucket_t;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The hashmap is a collection of buckets
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  bucket_t *bucket; // Array of bucket_t
  size_t capacity;  // Number of buckets
  size_t nitems;    // Number of items in all the buckets
  size_t total_key_length;
} mph_t;



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Create a hashmap with the given number of buckets.
// 'capacity' cannot be changed once hashmap is created
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mph_t *mph_init(size_t capacity); 

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Free all memory associated with hashmap.
// This includes all cached copies of the keys
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void mph_destroy(mph_t *mph);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Add a key to the hashmap.
// The contents of key are copied and cached within the hashmap
//
// Note: currently there is no checking if the key is already in the hashmap
//       If you do happen to add duplicate keys, only the value for the 
//       first key will ever be returned.
//
// @param mph mph_t object
// @param key byte data
// @param len length of data in 'key'
// @return boolean. Success?
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool mph_set(mph_t *mph, uint8_t *key, size_t len);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Lookup a key in the hashmap.
//
// @param mph mph_t object
// @param key byte data
// @param len length of data in 'key'
// @return the int32_t value that is stored by this key
//         If key is not in hashmap, return MPH_NOT_FOUND
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int32_t mph_get(mph_t *mph, uint8_t *key, size_t len);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Get the index for a value, adding it to the hashmap if not already present
//
// @param mph mph_t object
// @param key byte data
// @param len length of data in 'key'
// @return the int32_t value that is stored by this key
//         Returns index or MPH_ERROR
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int32_t mph_get_set(mph_t *mph, uint8_t *key, size_t len);


