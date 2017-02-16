#ifndef _DICT_H_
#define _DICT_H_

#include "list.h"

// 2^9 = 512 buckets should be enough for most cases
#define BUCKET_NUM_PWR 9
#define BUCKET_NUM (1<<BUCKET_NUM_PWR)

// A hash-table based implementation of dictionary.
typedef struct Dict {
  ListHead *buckets; // Uses link-list to deal with conflicts.
} Dict;
typedef ListData DictData;

// Initializes a new dict.
int dict_init(Dict *dict);

// Adds an entry to dict. Returns non-zero on failure.
int dict_add(Dict *dict, DictData data);

// Looks up an entry in dict. Returns NULL if the key's not in dict.
DictData *dict_look_up(Dict *dict, char *key, size_t len);

// Destroys a dict, and releases all the entries.
void dict_destroy(Dict *dict);

#endif
