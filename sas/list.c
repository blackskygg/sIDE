#include "list.h"

#include <string.h>

void list_init(ListHead *head) {
  memset(head, 0, sizeof(ListHead));
  head->next = head;
  head->prev = head;
}

int list_push_back(ListHead *head, ListData data) {
  ListEntry *new_entry = malloc(sizeof(ListEntry));
  if (!new_entry) goto ERROR;

  // Set all fields to zero so that we could call free()s at any point.
  memset(new_entry, 0, sizeof(ListEntry));
      
  // Make a copy of data
  new_entry->data.key = malloc(data.key_len);
  if (!new_entry->data.key) goto ERROR;
  memcpy(new_entry->data.key, data.key, data.key_len);
  new_entry->data.key_len = data.key_len;

  new_entry->data.value = malloc(data.val_len);
  if (!new_entry->data.value) goto ERROR;
  memcpy(new_entry->data.value, data.value, data.val_len);
  new_entry->data.val_len = data.val_len;

  // Push back
  new_entry->prev = head->prev;
  head->prev->next = new_entry;
  new_entry->next = head;
  head->prev = new_entry;
  
  return 0;
  
ERROR:
  free(new_entry->data.key);
  free(new_entry->data.value);
  free(new_entry);
  return -1;
}


ListEntry *list_pop_front(ListHead *head) {
  if (list_is_empty(head)) return NULL;

  ListEntry *result;
  
  result = head->next;
  head->next->next->prev = head;
  head->next = head->next->next;

  return result;
}

int list_iterate(ListHead *head,
                 int (*fp)(ListEntry *, void *data),
                 void *data) {
  ListEntry *curr, *next;
  int ret_val;

  curr = head->next;
  // Pre-fetch next to ensure safety
  next = curr->next;
  while (curr != head) {
    ret_val = (*fp)(curr, data);
    if (ret_val) return ret_val; 
    
    curr = next;
    next = curr->next;
  }

  return 0;
}

void list_free_entry(ListEntry *entry) {
  free(entry->data.key);
  free(entry->data.value);
  free(entry);
}

// Iteration callback for destroying a whole list
static int _list_free_entry(ListEntry *entry, void *dummy) {
  list_free_entry(entry);
  return 0;
}

void list_destroy(ListHead *head) {
  list_iterate(head, _list_free_entry, NULL);
}
