


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Buckets
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define BUCKET_START_CAPACITY 1
typedef struct {
  uint8_t **key;   // Array: pointer to the keys (hash retrains a copy of original key)
  size_t *len;     // Array: the lengths of each key
  uint32_t *hash;  // Array: hash of the key
  int32_t *value;   // Array: integer value for each key i.e. index of insertion order
  size_t nitems;   // the number of items in this bucket
  size_t capacity; // the capacity of this bucket (for triggering re-alloc)
} bucket_t;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The hashmap is a collection of buckets
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  bucket_t *bucket;    // Array of bucket_t
  size_t nbuckets;     // Number of buckets
  size_t total_items;  // Number of items in all the buckets
} mph_t;



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Create a hashmap with the given number of buckets.
// 'nbuckets' cannot be changed once hashmap is created
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mph_t *mph_init(size_t nbuckets); 

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
//       as my current workflows would only call mph_add after a failed lookup
//       If you do happen to add duplicate keys, only the value for the 
//       first key will ever be returned.
//
// @param mph mph_t object
// @param key byte data
// @param len length of data in 'key'
// @return the int32_t value that is stored by this key
//         Caller must check return value. If <0 then an error has occurred
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int32_t mph_add(mph_t *mph, uint8_t *key, size_t len);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Lookup a key in the hashmap.
//
// @param mph mph_t object
// @param key byte data
// @param len length of data in 'key'
// @return the int32_t value that is stored by this key
//         If key is not in hashmap, return -1
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int32_t mph_lookup(mph_t *mph, uint8_t *key, size_t len);


