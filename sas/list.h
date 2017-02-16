#ifndef _LIST_H_
#define _LIST_H_

#include <stddef.h>
#include <malloc.h>

// The list stores key-value pairs
typedef struct ListData {
  char *key;
  size_t key_len; 
  void *value;
  size_t val_len;
}ListData;

// A typical doubly linked list with a head node
typedef struct ListEntry {
  struct ListEntry *next;
  struct ListEntry *prev;
  struct ListData data;
} ListEntry, ListHead;

#define list_is_empty(head) ((head)->next == (head)->prev)

// Initialize a new list.
void list_init(ListHead *head);

// Appends an entry to the back of a list, returns non-zero on error.
// The data is deep-copied.
int list_push_back(ListHead *head, ListData data);

// Removes the first entry, returns NULL if empty.
// It's the caller's responsibility to free the resources of the poped entry.
ListEntry *list_pop_front(ListHead *head);

// Iterates through list and calls fp on every entry.
// This function stops when fp() returns non-zero, and returns the retval of fp().
// It's safe for insertion and removal.
int list_iterate(
    ListHead *head,
    int (*fp)(ListEntry *, void *data),
    void *data);

// Releases resources of an entry allocated by push_back.
void list_free_entry(ListEntry *entry);

// Delete all the entries in a list and free up all the spaces.
void list_destroy(ListHead *head);

#endif
