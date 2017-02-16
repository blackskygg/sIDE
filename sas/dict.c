#include "dict.h"

#include <stdint.h>
#include <string.h>

// Murmurhash3(32-bit version on little-endian machine).
// The function takes a chunk of data and calculates a 32-bit hash value.
// The hash is masked by the HASH_BITMASK, so retval is within [0, BUCKET_NUM).
uint32_t murmur3_hash(const uint8_t* key, size_t len) {
  const uint32_t HASH_SEED = 5381;   // Seed for the hash function.
  // Since the uint32_t version murmurhash3,
  // we have to mask the value to fit into our hash table.
  const uint32_t HASH_BITMASK = BUCKET_NUM - 1;

  uint32_t hash = HASH_SEED;
  const uint32_t c1 = 0xcc9e2d51, c2 = 0x1b873593;
  const uint32_t n = 0xe6546b64;

  // Process every 4-byte chunks of key.
  if (len > 3) {
    const uint32_t* key_x4 = (const uint32_t*) key;
    size_t i = len >> 2;
    
    do {
      uint32_t k = *key_x4++;
      k *= c1;
      k = (k << 15) | (k >> 17);
      k *= c2;
      hash ^= k;
      hash = (hash << 13) | (hash >> 19);
      hash += (hash << 2) + n;
    } while (--i);
    key = (const uint8_t*) key_x4;
  }

  // Process the remaining data.
  if (len & 3) {
    size_t i = len & 3;
    uint32_t k = 0;
    
    key = &key[i - 1];
    do {
      k <<= 8;
      k |= *key--;
    } while (--i);
    
    k *= c1;
    k = (k << 15) | (k >> 17);
    k *= c2;
    hash ^= k;
  }
  
  hash ^= len;
  hash ^= hash >> 16;
  hash *= 0x85ebca6b;
  hash ^= hash >> 13;
  hash *= 0xc2b2ae35;
  hash ^= hash >> 16;
  
  return hash & HASH_BITMASK;
}

int dict_init(Dict *dict) {
  dict->buckets = malloc(BUCKET_NUM * sizeof(ListEntry));
  if (!dict->buckets) return -1;

  // Init buckets.
  for (int i = 0; i < BUCKET_NUM; ++i) {
    list_init(dict->buckets+i);
  }
  return 0;
}

void dict_destroy(Dict *dict) {
  for (int i = 0; i < BUCKET_NUM; ++i) {
    list_destroy(dict->buckets+i);
  }

  free(dict->buckets);
}

int dict_add(Dict *dict, DictData data) {
  uint32_t hash = murmur3_hash((uint8_t*)data.key, data.key_len);
  return list_push_back(&dict->buckets[hash], data);
}

// Info passed to the callback when iterating through a list to look up things.
typedef struct LookUpInfo {
  char *key;
  size_t len;
  ListData *target;
} LookUpInfo;

// Callback function for list_iterate to look up an entry.
static int look_up_callback(ListEntry *entry, void *info) {
  LookUpInfo *lookup_info = info;

  if (entry->data.key_len == lookup_info->len) {
    if (!memcmp(entry->data.key, lookup_info->key, lookup_info->len)) {
      lookup_info->target = &entry->data;
      return 1; // Target found! Stops the iteration.
    }
  }
  
  return 0;
}

DictData *dict_look_up(Dict *dict, char *key, size_t len) {
  uint32_t hash = murmur3_hash((uint8_t *)key, len);
  ListHead *bucket = &dict->buckets[hash];
  LookUpInfo info = {.key = key, .len = len, .target = NULL};
  
  list_iterate(bucket, look_up_callback, &info);
  
  return info.target;
}
